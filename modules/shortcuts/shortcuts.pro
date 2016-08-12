
include(../../common.pri)

TEMPLATE        = lib
CONFIG         += plugin c++11 link_pkgconfig
QT             += widgets dbus
INCLUDEPATH    += ../../frame/ ../../widgets
PKGCONFIG += dtkbase dtkwidget
LIBS += -L../../widgets -lwidgets

TARGET          = $$qtLibraryTarget(shortcuts)
DESTDIR         = $$_PRO_FILE_PWD_/../

HEADERS += \
    mainwidget.h \
    shortcuts.h \
    shortcutwidget.h \
    tooltip.h \
    selectdialog.h \
    shortcutdbus.h \
    shortcutedit.h

SOURCES += \
    mainwidget.cpp \
    shortcuts.cpp \
    shortcutwidget.cpp \
    tooltip.cpp \
    selectdialog.cpp \
    shortcutdbus.cpp \
    shortcutedit.cpp

DISTFILES += shortcuts.json
RESOURCES += \
    theme.qrc

target.path = $${PREFIX}/lib/dde-control-center/modules/
INSTALLS += target

ARCH = $$QMAKE_HOST.arch
isEqual(ARCH, mips64) | isEqual(ARCH, mips32) | isEqual(ARCH, sw_64) {
    DEFINES += SHORTCUT_DISABLE_TERMINAL_QUAKE
    DEFINES += SHORTCUT_DISABLE_WM_SWITCHER
}

