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

#include "platform_unityplugin.h"

#include <QtPlugin>

#include <extensionsystem/pluginmanager.h>

namespace {
    static const char *QTCREATOR_DESKTOP_FILE = "qtcreator.desktop";
}

namespace PlatformUnity {
namespace Internal {

PlatformUnityPlugin::PlatformUnityPlugin()
    : m_unityLibHandle(0)
    , m_totalProgress(0)
    , m_inspector(0)
    , m_qtcreatorEntry(0)
    , m_get_unity_running(0)
    , m_entry_set_count(0)
    , m_entry_set_count_visible(0)
    , m_entry_set_progress(0)
    , m_entry_set_progress_visible(0)
{
    ExtensionSystem::PluginManager::addObject(this);
}

PlatformUnityPlugin::~PlatformUnityPlugin()
{
    if (m_unityLibHandle)
        dlclose(m_unityLibHandle);
    ExtensionSystem::PluginManager::removeObject(this);
}

bool PlatformUnityPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    m_unityLibHandle = dlopen("libunity.so.4", RTLD_LAZY);
    if (!m_unityLibHandle)
        m_unityLibHandle = dlopen("libunity.so.6", RTLD_LAZY);
    if (!m_unityLibHandle)
        m_unityLibHandle = dlopen("libunity.so.9", RTLD_LAZY);
    if (!m_unityLibHandle)
        return false;

    unity_inspector_get_default_func inspector_get_default =
            reinterpret_cast<unity_inspector_get_default_func>(dlsym(m_unityLibHandle, "unity_inspector_get_default"));

    if (!inspector_get_default)
        return false;

    m_inspector = inspector_get_default();
    m_get_unity_running =
            reinterpret_cast<unity_inspector_get_unity_running_func>(dlsym(m_unityLibHandle, "unity_inspector_get_unity_running"));

    unity_launcher_entry_get_for_desktop_id_func entry_get_for_desktop_id =
            reinterpret_cast<unity_launcher_entry_get_for_desktop_id_func>(
                dlsym(m_unityLibHandle, "unity_launcher_entry_get_for_desktop_id"));

    if (entry_get_for_desktop_id) {
        m_qtcreatorEntry = entry_get_for_desktop_id(QTCREATOR_DESKTOP_FILE);

        m_entry_set_count =
                reinterpret_cast<unity_launcher_entry_set_count_func>(
                    dlsym(m_unityLibHandle, "unity_launcher_entry_set_count"));

        m_entry_set_count_visible =
                reinterpret_cast<unity_launcher_entry_set_count_visible_func>(
                    dlsym(m_unityLibHandle, "unity_launcher_entry_set_count_visible"));

        m_entry_set_progress =
                reinterpret_cast<unity_launcher_entry_set_progress_func>(
                    dlsym(m_unityLibHandle, "unity_launcher_entry_set_progress"));

        m_entry_set_progress_visible =
                reinterpret_cast<unity_launcher_entry_set_progress_visible_func>(
                    dlsym(m_unityLibHandle, "unity_launcher_entry_set_progress_visible"));
    }
    Q_ASSERT(m_entry_set_count && m_entry_set_count_visible && m_entry_set_progress && m_entry_set_progress_visible);
    return true;
}

void PlatformUnityPlugin::extensionsInitialized()
{
}

void PlatformUnityPlugin::setApplicationLabel(const QString &text)
{
    if (m_qtcreatorEntry && m_entry_set_count && m_entry_set_count_visible) {
        // unity can only handle numbers...
        int count = text.toInt();
        m_entry_set_count(m_qtcreatorEntry, count);
        m_entry_set_count_visible(m_qtcreatorEntry, count != 0);
    }
}

void PlatformUnityPlugin::setApplicationProgressRange(int min, int max)
{
    m_totalProgress = max-min;
}

void PlatformUnityPlugin::setApplicationProgressValue(int value)
{
    if (m_qtcreatorEntry && m_entry_set_progress)
        m_entry_set_progress(m_qtcreatorEntry, value/(double)m_totalProgress);
}

void PlatformUnityPlugin::setApplicationProgressVisible(bool visible)
{
    if (m_qtcreatorEntry && m_entry_set_progress_visible)
        m_entry_set_progress_visible(m_qtcreatorEntry, visible);
}

} // namespace Internal
} // namespace HelloWorld

Q_EXPORT_PLUGIN(PlatformUnity::Internal::PlatformUnityPlugin)
