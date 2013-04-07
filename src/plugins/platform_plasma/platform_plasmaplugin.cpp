/****************************************************************************
**
** Copyright (C) 2013 Daniel Molkentin, Mirko Boehm.
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtPlugin>

#include "platform_plasmaplugin.h"

#include <extensionsystem/pluginmanager.h>

#include <kuiserverjobtracker.h>

#include "BuildJob.h"

namespace PlatformPlasma {
namespace Internal {

PlatformPlasmaPlugin::PlatformPlasmaPlugin()
    : m_jobTracker(new KUiServerJobTracker(this))
    , m_buildJob(0)
{
    ExtensionSystem::PluginManager::addObject(this);
}

PlatformPlasmaPlugin::~PlatformPlasmaPlugin()
{
    ExtensionSystem::PluginManager::removeObject(this);
}

bool PlatformPlasmaPlugin::initialize(const QStringList&, QString*)
{
    return true;
}

void PlatformPlasmaPlugin::extensionsInitialized()
{
    if (!m_buildJob) return;

}

void PlatformPlasmaPlugin::setApplicationLabel(const QString& text)
{
    if (!m_buildJob) return;
    m_buildJob->setApplicationLabel(text);
}

void PlatformPlasmaPlugin::setApplicationProgressRange(int min, int max)
{
    if (!m_buildJob) return;
    m_buildJob->setApplicationProgressRange(min, max);
}

void PlatformPlasmaPlugin::setApplicationProgressValue(int progress)
{
    if (!m_buildJob) return;
    m_buildJob->setApplicationProgressValue(progress);
}

void PlatformPlasmaPlugin::setApplicationProgressVisible(const QString &projectName, bool visible)
{
    if (visible) {
        if (m_buildJob == 0) {
            m_buildJob = new BuildJob(this);
            m_buildJob->setProjectName(projectName);
            m_jobTracker->registerJob(m_buildJob);
            m_buildJob->registered();
        }
    } else {
        if (m_buildJob!=0) {
            m_buildJob->completed();
            delete m_buildJob;
            m_buildJob = 0;
        }
    }
}

} // namespace Internal
} // namespace HelloWorld

Q_EXPORT_PLUGIN(PlatformPlasma::Internal::PlatformPlasmaPlugin)
