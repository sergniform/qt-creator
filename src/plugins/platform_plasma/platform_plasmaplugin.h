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

#ifndef PLATFORM_PLASMA_PLUGIN_H
#define PLATFORM_PLASMA_PLUGIN_H

#include <glib-2.0/glib.h>
#include <dlfcn.h>

#include <coreplugin/iplatformintegration.h>

class KUiServerJobTracker;

namespace PlatformPlasma {

namespace Internal {

class BuildJob;

class PlatformPlasmaPlugin : public Core::IPlatformIntegration
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PlatformPlasma.json")

public:
    PlatformPlasmaPlugin();
    ~PlatformPlasmaPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

    //implement Core::IPlatformIntegration
    void setApplicationLabel(const QString &text);
    void setApplicationProgressRange(int min, int max);
    void setApplicationProgressValue(int value);
    void setApplicationProgressVisible(const QString& projectName, bool visible);

private:
    KUiServerJobTracker* m_jobTracker;
    BuildJob* m_buildJob;
};

} // namespace Internal
} // namespace PlatformPlasma

#endif // PLATFORM_PLASMA_PLUGIN_H
