/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "loadcoredialog.h"

#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerstartparameters.h"

#include <coreplugin/icore.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/profilechooser.h>
#include <projectexplorer/profileinformation.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <ssh/sftpdefs.h>
#include <ssh/sftpfilesystemmodel.h>
#include <utils/historycompleter.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QRegExp>
#include <QTemporaryFile>

#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBrowser>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// SelectRemoteFileDialog
//
///////////////////////////////////////////////////////////////////////

class SelectRemoteFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectRemoteFileDialog(QWidget *parent);

    void attachToDevice(Profile *profile);
    QString localFile() const { return m_localFile; }
    QString remoteFile() const { return m_remoteFile; }

private slots:
    void handleSftpOperationFinished(QSsh::SftpJobId, const QString &error);
    void handleSftpOperationFailed(const QString &errorMessage);
    void handleConnectionError(const QString &errorMessage);
    void handleRemoteError(const QString &errorMessage);
    void selectFile();

private:
    QSortFilterProxyModel m_model;
    SftpFileSystemModel m_fileSystemModel;
    QTreeView *m_fileSystemView;
    QTextBrowser *m_textBrowser;
    QDialogButtonBox *m_buttonBox;
    QString m_localFile;
    QString m_remoteFile;
    SftpJobId m_sftpJobId;
};

SelectRemoteFileDialog::SelectRemoteFileDialog(QWidget *parent)
    : QDialog(parent)
{
    m_model.setSourceModel(&m_fileSystemModel);

    m_fileSystemView = new QTreeView(this);
    m_fileSystemView->setModel(&m_model);
    m_fileSystemView->setSortingEnabled(true);
    m_fileSystemView->setUniformRowHeights(true);
    m_fileSystemView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileSystemView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileSystemView->header()->setDefaultSectionSize(100);
    m_fileSystemView->header()->setStretchLastSection(true);

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setEnabled(false);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_fileSystemView);
    layout->addWidget(m_textBrowser);
    layout->addWidget(m_buttonBox);

    QObject::connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));
    QObject::connect(m_buttonBox, SIGNAL(accepted()), SLOT(selectFile()));

    connect(&m_fileSystemModel, SIGNAL(sftpOperationFailed(QString)),
            SLOT(handleSftpOperationFailed(QString)));
    connect(&m_fileSystemModel, SIGNAL(connectionError(QString)),
            SLOT(handleConnectionError(QString)));
}

void SelectRemoteFileDialog::attachToDevice(Profile *profile)
{
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    QTC_ASSERT(profile, return);
    IDevice::ConstPtr device = DeviceProfileInformation::device(profile);
    QTC_ASSERT(device, return);
    QSsh::SshConnectionParameters sshParams = device->sshParameters();
    m_fileSystemModel.setSshConnection(sshParams);
}

void SelectRemoteFileDialog::handleSftpOperationFailed(const QString &errorMessage)
{
    m_textBrowser->append(errorMessage);
    //reject();
}

void SelectRemoteFileDialog::handleConnectionError(const QString &errorMessage)
{
    m_textBrowser->append(errorMessage);
    //reject();
}

void SelectRemoteFileDialog::handleSftpOperationFinished(QSsh::SftpJobId, const QString &error)
{
    if (error.isEmpty()) {
        m_textBrowser->append(tr("Download of remote file succeeded."));
        accept();
    } else {
        m_textBrowser->append(error);
        //reject();
    }
}

void SelectRemoteFileDialog::handleRemoteError(const QString &errorMessage)
{
    m_textBrowser->append(errorMessage);
}

void SelectRemoteFileDialog::selectFile()
{
    const QModelIndexList indexes =
            m_fileSystemView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;

    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_fileSystemView->setEnabled(false);

    connect(&m_fileSystemModel, SIGNAL(sftpOperationFinished(QSsh::SftpJobId,QString)),
            SLOT(handleSftpOperationFinished(QSsh::SftpJobId,QString)));

    {
        QTemporaryFile localFile(QDir::tempPath() + QLatin1String("/remotecore-XXXXXX"));
        localFile.open();
        m_localFile = localFile.fileName();
    }

    QModelIndex idx = indexes.at(0);
    idx = idx.sibling(idx.row(), 1);
    m_remoteFile = m_fileSystemModel.data(idx, SftpFileSystemModel::PathRole).toString();
    m_sftpJobId = m_fileSystemModel.downloadFile(idx, m_localFile);
}

///////////////////////////////////////////////////////////////////////
//
// AttachCoreDialog
//
///////////////////////////////////////////////////////////////////////

class AttachCoreDialogPrivate
{
public:
    ProfileChooser *profileChooser;

    QSettings *settings;

    PathChooser *localExecFileName;
    PathChooser *localCoreFileName;
    QLineEdit *remoteCoreFileName;
    QPushButton *selectRemoteCoreButton;

    PathChooser *overrideStartScriptFileName;

    QDialogButtonBox *buttonBox;
};

AttachCoreDialog::AttachCoreDialog(QWidget *parent)
    : QDialog(parent), d(new AttachCoreDialogPrivate)
{
    setWindowTitle(tr("Load Core File"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    d->settings = ICore::settings();

    d->profileChooser = new ProfileChooser(this);

    d->selectRemoteCoreButton = new QPushButton(tr("Browse..."), this);
    d->remoteCoreFileName = new QLineEdit(this);
    d->remoteCoreFileName->setEnabled(false);

    d->localCoreFileName = new PathChooser(this);
    d->localCoreFileName->setExpectedKind(PathChooser::File);
    d->localCoreFileName->setPromptDialogTitle(tr("Select Core File"));

    d->localExecFileName = new PathChooser(this);
    d->localExecFileName->setExpectedKind(PathChooser::File);
    d->localExecFileName->setPromptDialogTitle(tr("Select Executable"));

    d->overrideStartScriptFileName = new PathChooser(this);
    d->overrideStartScriptFileName->setExpectedKind(PathChooser::File);
    d->overrideStartScriptFileName->setPromptDialogTitle(tr("Select Startup Script"));

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    d->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QHBoxLayout *coreLayout = new QHBoxLayout;
    coreLayout->addWidget(d->localCoreFileName);
    coreLayout->addWidget(d->remoteCoreFileName);
    coreLayout->addWidget(d->selectRemoteCoreButton);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(6);
    formLayout->setVerticalSpacing(6);
    formLayout->addRow(tr("Target:"), d->profileChooser);
    formLayout->addRow(tr("&Executable:"), d->localExecFileName);
    formLayout->addRow(tr("Core file:"), coreLayout);
    formLayout->addRow(tr("Override &start script:"), d->overrideStartScriptFileName);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    formLayout->addRow(d->buttonBox);

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    vboxLayout->addLayout(formLayout);
    vboxLayout->addStretch();
    vboxLayout->addWidget(line);
    vboxLayout->addWidget(d->buttonBox);

    connect(d->selectRemoteCoreButton, SIGNAL(clicked()), SLOT(selectRemoteCoreFile()));
    connect(d->remoteCoreFileName, SIGNAL(textChanged(QString)), SLOT(changed()));
    connect(d->profileChooser, SIGNAL(activated(int)), SLOT(changed()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete d;
}

bool AttachCoreDialog::isLocal() const
{
    Profile *profile = d->profileChooser->currentProfile();
    QTC_ASSERT(profile, return false);
    IDevice::ConstPtr device = DeviceProfileInformation::device(profile);
    QTC_ASSERT(device, return false);
    SshConnectionParameters sshParams = device->sshParameters();
    d->settings->setValue(QLatin1String("LastProfile"),
        d->profileChooser->currentProfileId().toString());
    return sshParams.host.isEmpty();
}

void AttachCoreDialog::changed()
{
    bool isValid = d->profileChooser->currentIndex() >= 0
            && !localCoreFile().isEmpty();
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);

    if (isLocal()) {
        d->localCoreFileName->setVisible(true);
        d->remoteCoreFileName->setVisible(false);
        d->selectRemoteCoreButton->setVisible(false);
    } else {
        d->remoteCoreFileName->setVisible(true);
        d->selectRemoteCoreButton->setVisible(true);
        d->localCoreFileName->setVisible(false);
    }
}

void AttachCoreDialog::selectRemoteCoreFile()
{
    changed();
    QTC_ASSERT(!isLocal(), return);
    SelectRemoteFileDialog dlg(this);
    dlg.setWindowTitle(tr("Select Remote Core File"));
    dlg.attachToDevice(d->profileChooser->currentProfile());
    if (dlg.exec() == QDialog::Rejected)
        return;
    d->localCoreFileName->setPath(dlg.localFile());
    d->remoteCoreFileName->setText(dlg.remoteFile());
    changed();
}

QString AttachCoreDialog::localCoreFile() const
{
    return d->localCoreFileName->path();
}

QString AttachCoreDialog::localExecutableFile() const
{
    return d->localExecFileName->path();
}

void AttachCoreDialog::setLocalExecutableFile(const QString &fileName)
{
    d->localExecFileName->setPath(fileName);
}

void AttachCoreDialog::setLocalCoreFile(const QString &fileName)
{
    d->localCoreFileName->setPath(fileName);
}

void AttachCoreDialog::setRemoteCoreFile(const QString &fileName)
{
    d->remoteCoreFileName->setText(fileName);
}

QString AttachCoreDialog::remoteCoreFile() const
{
    return d->remoteCoreFileName->text();
}

void AttachCoreDialog::setProfileId(const Core::Id &id)
{
    d->profileChooser->setCurrentProfileId(id);
    changed();
}

Profile *AttachCoreDialog::profile() const
{
    return d->profileChooser->currentProfile();
}

QString AttachCoreDialog::overrideStartScript() const
{
    return d->overrideStartScriptFileName->path();
}

void AttachCoreDialog::setOverrideStartScript(const QString &scriptName)
{
    d->overrideStartScriptFileName->setPath(scriptName);
}

} // namespace Internal
} // namespace Debugger

#include "loadcoredialog.moc"