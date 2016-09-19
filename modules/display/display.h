/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <QObject>
#include <QtPlugin>

#include <darrowlineexpand.h>
#include <dwidget_global.h>

#include "interfaces.h"

#include "dbus/displayinterface.h"

DWIDGET_USE_NAMESPACE

class QFrame;
class MonitorGround;
class MonitorInterface;
class Monitor;
class ListWidget;
class CustomSettings;
class ScrollFrame;

class Display: public QObject
{
    Q_OBJECT

public:
    Display(DisplayInterface *dbus, QObject *parent = NULL);
    QFrame *getContent();

private slots:
    void init();
    void updateUI();
    void onDisplayModeChanged();

private:
    ScrollFrame *m_frame = NULL;
    DisplayInterface *m_dbusDisplay;
    MonitorGround *m_monitorGround;
    QList<MonitorInterface *> m_dbusMonitors;
    QList<Monitor *> m_monitors;
    ListWidget *m_widgetList = NULL;
    QStringList m_monitorNameList;
    CustomSettings *m_singleSettings = NULL;
};

class DisplayModule: public QObject, ModuleInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.ControlCenter.ModuleInterface" FILE "display.json")
    Q_INTERFACES(ModuleInterface)

public:
    DisplayModule();
    QFrame *getContent() Q_DECL_OVERRIDE;

private:
    DisplayInterface    *m_dbus = NULL;
    Display             *m_display = NULL;
};

#endif // DISPLAY_H

