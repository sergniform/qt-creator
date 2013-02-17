TEMPLATE = lib
TARGET = PlatformUnity

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)

HEADERS += platform_unityplugin.h
SOURCES += platform_unityplugin.cpp

OTHER_FILES += PlatformUnity.pluginspec.in
