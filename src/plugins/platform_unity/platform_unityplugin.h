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

#ifndef PLATFORM_UNITY_PLUGIN_H
#define PLATFORM_UNITY_PLUGIN_H

#include <glib-2.0/glib.h>
#include <dlfcn.h>

#include <coreplugin/iplatformintegration.h>

namespace PlatformUnity {
namespace Internal {

//Not published, this header is never included anywhere outside of the plugin
typedef struct _UnityInspector UnityInspector;
typedef UnityInspector* (*unity_inspector_get_default_func)(void);
typedef gboolean (*unity_inspector_get_unity_running_func)(UnityInspector* self);

typedef struct _UnityLauncherEntry UnityLauncherEntry;
typedef UnityLauncherEntry* (*unity_launcher_entry_get_for_desktop_id_func)(const gchar* desktop_id);
typedef void (*unity_launcher_entry_set_count_func)(UnityLauncherEntry* self, gint64 value);
typedef void (*unity_launcher_entry_set_count_visible_func)(UnityLauncherEntry* self, gboolean value);
typedef void (*unity_launcher_entry_set_progress_func)(UnityLauncherEntry* self, gdouble value);
typedef void (*unity_launcher_entry_set_progress_visible_func)(UnityLauncherEntry* self, gboolean value);

class PlatformUnityPlugin : public Core::IPlatformIntegration
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PlatformUnity.json")

public:
    PlatformUnityPlugin();
    ~PlatformUnityPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

    //implement Core::IPlatformIntegration
    void setApplicationLabel(const QString &text);
    void setApplicationProgressRange(int min, int max);
    void setApplicationProgressValue(int value);
    void setApplicationProgressVisible(bool visible);

private:
    void *m_unityLibHandle;
    int m_totalProgress;
    UnityInspector* m_inspector;
    UnityLauncherEntry* m_qtcreatorEntry;
    unity_inspector_get_unity_running_func m_get_unity_running;
    unity_launcher_entry_set_count_func m_entry_set_count;
    unity_launcher_entry_set_count_visible_func m_entry_set_count_visible;
    unity_launcher_entry_set_progress_func m_entry_set_progress;
    unity_launcher_entry_set_progress_visible_func m_entry_set_progress_visible;
};

} // namespace Internal
} // namespace PlatformUnity

#endif // PLATFORM_UNITY_PLUGIN_H
