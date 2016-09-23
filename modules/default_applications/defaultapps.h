/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#ifndef DEFAULTAPPS_H
#define DEFAULTAPPS_H

#include <QObject>
#include <QtPlugin>
#include <QButtonGroup>
#include <QThread>
#include <QMap>
#include <QtConcurrent>

#include "moduleheader.h"

#include "interfaces.h"


#include "dbus/dbusdefaultapps.h"
#include "dbus/dbusdefaultmedia.h"

#include <darrowlineexpand.h>
#include <dexpandgroup.h>
#include <dswitchbutton.h>
#include <doptionlist.h>

DWIDGET_USE_NAMESPACE

class QLabel;
class QFrame;

class RunnableTask;

class DefaultApps : public QObject
{
    Q_OBJECT

public:
    DefaultApps(QObject *parent = NULL);
    ~DefaultApps();
    QFrame *getContent();

    enum DefaultAppsCategory {
        Browser, Mail, Text, Music, Video, Picture, Terminal,
        CD_Audio, DVD_Video, MusicPlayer, Camera, Software,
    };

private:
    DArrowLineExpand *createDefaultAppsExpand(const DefaultAppsCategory &category, DArrowLineExpand *defaultApps);
    const QString getTypeByCategory(const DefaultAppsCategory &category);
    const QStringList getTypeListByCategory(const DefaultAppsCategory &category);
    void setMediaOptionVisible(const bool visible);
    void resetDefaults();
    void updateListCheckedIndex();
    void updateCheckedItem(const DefaultAppsCategory &category);

    void createTask();

private:
    bool isMediaApps(const DefaultAppsCategory &category) const;

private slots:
    void lazyLoad();
    void arrowLineExpandSetContent(QJsonArray json, int acategory, DArrowLineExpand *arrowLineApps);

private:
    QFrame *m_centralWidget;

    ModuleHeader *m_header;

    DBusDefaultApps m_dbusDefaultApps;
    DBusDefaultMedia *m_dbusDefaultMedia;

    DExpandGroup *m_appGrp;

    DArrowLineExpand *m_modBrowser = nullptr;
    DArrowLineExpand *m_modMail = nullptr;
    DArrowLineExpand *m_modText = nullptr;
    DArrowLineExpand *m_modMusic = nullptr;
    DArrowLineExpand *m_modVideo = nullptr;
    DArrowLineExpand *m_modPicture = nullptr;
    DArrowLineExpand *m_modTerminal = nullptr;

#ifdef DCC_MEDIA_AUTOPLAY
    DExpandGroup *m_mediaGrp;
    DArrowLineExpand *m_modCDAudio = nullptr;
    DArrowLineExpand *m_modDVDVideo = nullptr;
    DArrowLineExpand *m_modMusicPlayer = nullptr;
    DArrowLineExpand *m_modCamera = nullptr;
    DArrowLineExpand *m_modSoftware = nullptr;
    DSwitchButton *m_autoPlaySwitch;
#endif

    QMap<DefaultApps::DefaultAppsCategory, DOptionList *> m_appsBtnList;
    QMap<DefaultApps::DefaultAppsCategory, DArrowLineExpand *> m_taskMap;

    QElapsedTimer m_timer;
};

class DBusDefaultMediaThread : public QObject
{
    Q_OBJECT

public:
    DBusDefaultMediaThread(DBusDefaultMedia *dbus);
    void run();
signals:
    void dbusConnected();

private:
    DBusDefaultMedia *m_dbus;
};

class SetDefAppsThread : public QObject
{
    Q_OBJECT

public:
    SetDefAppsThread(DBusDefaultApps *dbus, const QString &mime, const QString &appName, const QStringList &list);
    void run();

private:
    DBusDefaultApps *dbus;
    QString mime;
    QString appName;
    QStringList list;
};

class SetDefMediaThread : public QObject
{
    Q_OBJECT

public:
    SetDefMediaThread(DBusDefaultMedia *dbus, const QString &mime, const QString &appName, const QStringList &list);
    void run();

private:
    DBusDefaultMedia *dbus;
    QString mime;
    QString appName;
    QStringList list;
};

class DefaultAppsModule : public QObject, ModuleInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.ControlCenter.ModuleInterface" FILE "default_applications.json")
    Q_INTERFACES(ModuleInterface)

public:
    QFrame *getContent() Q_DECL_OVERRIDE;

private:
    DBusDefaultMedia m_dbusDefaultMedia;
    DefaultApps *m_defaultApps = NULL;
};
#endif
