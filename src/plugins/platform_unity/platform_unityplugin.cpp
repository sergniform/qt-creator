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

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/id.h>

#include <QDebug>
#include <QtPlugin>
#include <QAction>
#include <QMenu>

#include <glib-2.0/glib.h>
#include <dlfcn.h>

namespace {
    static const char *QTCREATOR_DESKTOP_FILE = "qt-creator.desktop";
    int total_progress = 0;

    // Unity data typedefs
    typedef struct _UnityInspector UnityInspector;
    typedef UnityInspector* (*unity_inspector_get_default_func)(void);
    typedef gboolean (*unity_inspector_get_unity_running_func)(UnityInspector* self);

    typedef struct _UnityLauncherEntry UnityLauncherEntry;
    typedef UnityLauncherEntry* (*unity_launcher_entry_get_for_desktop_id_func)(const gchar* desktop_id);
    typedef void (*unity_launcher_entry_set_count_func)(UnityLauncherEntry* self, gint64 value);
    typedef void (*unity_launcher_entry_set_count_visible_func)(UnityLauncherEntry* self, gboolean value);
    typedef void (*unity_launcher_entry_set_progress_func)(UnityLauncherEntry* self, gdouble value);
    typedef void (*unity_launcher_entry_set_progress_visible_func)(UnityLauncherEntry* self, gboolean value);

    UnityInspector* inspector = 0;
    // Reference to panel entry
    UnityLauncherEntry* qtcreatorEntry = 0;

    // Retrieved functions from libunity to go along with above typedefs
    unity_inspector_get_unity_running_func get_unity_running = 0;
    unity_launcher_entry_set_count_func entry_set_count = 0;
    unity_launcher_entry_set_count_visible_func entry_set_count_visible = 0;
    unity_launcher_entry_set_progress_func entry_set_progress = 0;
    unity_launcher_entry_set_progress_visible_func entry_set_progress_visible = 0;
}

namespace PlatformUnity {
namespace Internal {

PlatformUnityPlugin::PlatformUnityPlugin()
{
}

PlatformUnityPlugin::~PlatformUnityPlugin()
{
}

bool PlatformUnityPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    void* unity_lib = dlopen("libunity.so.4", RTLD_LAZY);
    if (!unity_lib)
        unity_lib = dlopen("libunity.so.6", RTLD_LAZY);
    if (!unity_lib)
        unity_lib = dlopen("libunity.so.9", RTLD_LAZY);
    if (!unity_lib)
        return false;

    unity_inspector_get_default_func inspector_get_default =
            reinterpret_cast<unity_inspector_get_default_func>(dlsym(unity_lib, "unity_inspector_get_default"));

    if (inspector_get_default) {
        inspector = inspector_get_default();

        get_unity_running =
                reinterpret_cast<unity_inspector_get_unity_running_func>(dlsym(unity_lib, "unity_inspector_get_unity_running"));
    }

    unity_launcher_entry_get_for_desktop_id_func entry_get_for_desktop_id =
            reinterpret_cast<unity_launcher_entry_get_for_desktop_id_func>(
                dlsym(unity_lib, "unity_launcher_entry_get_for_desktop_id"));

    if (entry_get_for_desktop_id) {
        qtcreatorEntry = entry_get_for_desktop_id(QTCREATOR_DESKTOP_FILE);

        entry_set_count =
                reinterpret_cast<unity_launcher_entry_set_count_func>(
                    dlsym(unity_lib, "unity_launcher_entry_set_count"));

        entry_set_count_visible =
                reinterpret_cast<unity_launcher_entry_set_count_visible_func>(
                    dlsym(unity_lib, "unity_launcher_entry_set_count_visible"));

        entry_set_progress =
                reinterpret_cast<unity_launcher_entry_set_progress_func>(
                    dlsym(unity_lib, "unity_launcher_entry_set_progress"));

        entry_set_progress_visible =
                reinterpret_cast<unity_launcher_entry_set_progress_visible_func>(
                    dlsym(unity_lib, "unity_launcher_entry_set_progress_visible"));
    }

    return true;
}

void PlatformUnityPlugin::extensionsInitialized()
{
}

void PlatformUnityPlugin::setApplicationLabel(const QString &text)
{
    if (qtcreatorEntry && entry_set_count && entry_set_count_visible) {
        // unity can only handle numbers...
        int count = text.toInt();
        entry_set_count(qtcreatorEntry, count);
        entry_set_count_visible(qtcreatorEntry, count != 0);
    }
}

void PlatformUnityPlugin::setApplicationProgressRange(int min, int max)
{
    total_progress = max-min;
}

void PlatformUnityPlugin::setApplicationProgressValue(int value)
{
    if (qtcreatorEntry && entry_set_progress)
        entry_set_progress(qtcreatorEntry, value/(double)total_progress);
}

void PlatformUnityPlugin::setApplicationProgressVisible(bool visible)
{
    if (qtcreatorEntry && entry_set_progress_visible)
        entry_set_progress_visible(qtcreatorEntry, visible);
}

} // namespace Internal
} // namespace HelloWorld

Q_EXPORT_PLUGIN(PlatformUnity::Internal::PlatformUnityPlugin)
