QT += widgets dbus svg concurrent

HEADERS    = frame.h \
    homescreen.h \
    modulemetadata.h \
    constants.h \
    sidebar.h \
    contentview.h \
    dtipsframe.h \
    dbus/dbuscontrolcenter.h \
    dbus/dbusxmousearea.h \
    ../modules/display/dbus/displayinterface.h \
    dbus/dbuscontrolcenterservice.h \
    pluginsmanager.h \
    dbus/dbusbluetooth.h \
    dbus/dbuswacom.h \
    controlcenterproxy.h \
    sidebarview.h \
    sidebarmodel.h \
    sidebardelegate.h \
    dbus/dbuslauncher.h
SOURCES    = frame.cpp \
             main.cpp \
    homescreen.cpp \
    sidebar.cpp \
    contentview.cpp \
    dtipsframe.cpp \
    dbus/dbuscontrolcenter.cpp \
    dbus/dbusxmousearea.cpp \
    ../modules/display/dbus/displayinterface.cpp \
    dbus/dbuscontrolcenterservice.cpp \
    pluginsmanager.cpp \
    dbus/dbuswacom.cpp \
    dbus/dbusbluetooth.cpp \
    controlcenterproxy.cpp \
    sidebarview.cpp \
    sidebarmodel.cpp \
    sidebardelegate.cpp \
    dbus/dbuslauncher.cpp

include(../common.pri)

TARGET     = dde-control-center
DESTDIR    = $$_PRO_FILE_PWD_/../

CONFIG += c++11 link_pkgconfig
LIBS += -L../widgets -lwidgets
PKGCONFIG += gtk+-2.0 dtkbase dtkutil dtkwidget x11

RESOURCES += \
    qss.qrc \
    images.qrc

INCLUDEPATH    += ../widgets ../modules/display

include(../interfaces/interfaces.pri)

isEqual(DCC_DISABLE_ANIMATION, YES){
    DEFINES += DCC_DISABLE_ANIMATION
}

