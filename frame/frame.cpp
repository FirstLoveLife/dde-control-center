/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <sys/mman.h>

#include "frame.h"
#include "homescreen.h"
#include "contentview.h"
#include "constants.h"
#include "dtipsframe.h"
#include "controlcenterproxy.h"
#include "sidebarview.h"
#include "dbus/displayinterface.h"
#include "dbus/dbuslauncher.h"

#include <QDir>
#include <QLibrary>
#include <QPluginLoader>
#include <QtWidgets>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHBoxLayout>
#include <QDebug>
#include <QDesktopWidget>

#include <dapplication.h>

DWIDGET_USE_NAMESPACE

Frame::Frame(QWidget *parent) :
    QFrame(parent),
    m_dbusXMouseArea(new DBusXMouseArea(this)),
    m_controlProxy(new ControlCenterProxy(this))
{
    QPalette palette;
    palette.setColor(QPalette::Background, DCC::BgLightColor);
    m_centeralWidget = new QWidget;
    m_centeralWidget->setPalette(palette);
    m_centeralWidget->setAutoFillBackground(true);

    m_homeScreen = new HomeScreen(m_centeralWidget);
    m_contentView = new ContentView(m_controlProxy, m_centeralWidget);

    m_leftShadow = new QWidget;
    m_leftShadow->setFixedWidth(DCC::FrameShadowWidth);
    m_leftShadow->setStyleSheet("background-image:url(:/resources/images/shadow_left.png) repeat-y;");
    m_rightShadow = new QWidget;
    m_rightShadow->setFixedWidth(DCC::FrameShadowWidth);
    m_rightShadow->setStyleSheet("background-image:url(:/resources/images/shadow_right.png) repeat-y;");

    QHBoxLayout *warpperLayout = new QHBoxLayout;
    warpperLayout->addWidget(m_leftShadow);
    warpperLayout->addWidget(m_centeralWidget);
    warpperLayout->addWidget(m_rightShadow);
    warpperLayout->setSpacing(0);
    warpperLayout->setMargin(0);

    m_centeralWarpper = new QWidget(this);
    m_centeralWarpper->setLayout(warpperLayout);

    m_showAni = new QPropertyAnimation(m_centeralWarpper, "pos");
    m_showAni->setDuration(DCC::FrameAnimationDuration);
    m_showAni->setEasingCurve(DCC::FrameShowCurve);

    m_hideAni = new QPropertyAnimation(m_centeralWarpper, "pos");
    m_hideAni->setDuration(DCC::FrameAnimationDuration);
    m_hideAni->setEasingCurve(DCC::FrameHideCurve);

    const int frameHeight = qApp->primaryScreen()->size().height();
    const int frameWidth = DCC::ControlCenterWidth + DCC::FrameShadowWidth;
    m_centeralWidget->setFixedSize(DCC::ControlCenterWidth, frameHeight);
    m_contentView->setFixedSize(DCC::ControlCenterWidth, frameHeight);
    m_homeScreen->setFixedSize(DCC::ControlCenterWidth, frameHeight);

    m_displayInter = new DisplayInterface(this);
//    DisplayInterface *display_dbus = new DisplayInterface(this);
    DBusLauncher *m_dbusLauncher = new DBusLauncher(this);

    m_posAdjustTimer = new QTimer(this);
    m_posAdjustTimer->setInterval(500);
    m_posAdjustTimer->setSingleShot(true);

//    auto updateGeometry = [display_dbus, this] {
//        QRect primaryRect = display_dbus->primaryRect();
//        const QString &primaryScreenName = display_dbus->primary();

//        qDebug() << "change screen, primary is: " << primaryScreenName;

//        if (primaryScreen)
//            disconnect(primaryScreen, &QScreen::geometryChanged, this, &Frame::updateGeometry);

//        for (const QScreen *screen : qApp->screens())
//        {
//            if (screen->name() == primaryScreenName) {
//                primaryScreen = screen;
//                primaryRect = screen->geometry();
//                connect(screen, &QScreen::geometryChanged, this, &Frame::updateGeometry);
//                break;
//            }
//        }

//        this->updateGeometry(primaryRect);
//    };

    connect(m_posAdjustTimer, &QTimer::timeout, this, &Frame::updateGeometry);
    connect(m_displayInter, &DisplayInterface::PrimaryRectChanged, m_posAdjustTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_displayInter, &DisplayInterface::ScreenHeightChanged, m_posAdjustTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_displayInter, &DisplayInterface::ScreenWidthChanged, m_posAdjustTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
//    connect(display_dbus, &DisplayInterface::PrimaryChanged, this, updateGeometry);
    connect(m_dbusLauncher, &DBusLauncher::Shown, [this] {hide(true);});

    connect(m_homeScreen, &HomeScreen::powerBtnClicked, [this] {hide(true);});
    connect(this, &Frame::hideInLeftChanged, m_posAdjustTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
//    connect(m_dbusXMouseArea, &DBusXMouseArea::ButtonRelease, this, &Frame::globalMouseReleaseEvent);
    connect(m_hideAni, &QPropertyAnimation::finished, this, &QFrame::hide, Qt::QueuedConnection);
    connect(m_hideAni, &QPropertyAnimation::valueChanged, this, &Frame::xChanged, Qt::QueuedConnection);
    connect(m_showAni, &QPropertyAnimation::valueChanged, this, &Frame::xChanged, Qt::QueuedConnection);
    connect(m_showAni, &QPropertyAnimation::finished, m_centeralWarpper, static_cast<void (QWidget::*)()>(&QWidget::update), Qt::QueuedConnection);
    connect(m_homeScreen, &HomeScreen::moduleSelected, this, &Frame::selectModule);
    connect(m_contentView, &ContentView::shutdownSelected, m_homeScreen, &HomeScreen::powerButtonClicked, Qt::DirectConnection);
    connect(m_contentView, &ContentView::shutdownSelected, [this]() -> void {hide();});
    connect(m_homeScreen, &HomeScreen::showAniFinished, m_contentView, &ContentView::unloadPlugin);
    connect(this, &Frame::hideInLeftChanged, m_contentView, &ContentView::reLayout);
    connect(m_contentView, &ContentView::backToHome, [this] {selectModule("home");});

    setFixedSize(frameWidth, frameHeight);
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setHideInLeft(false);

    updateGeometry();

#ifdef DCC_PRELOAD_MODULE_BACKEND
    setCursor(Qt::WaitCursor);
    PluginLoader *pl = new PluginLoader();
    m_pluginLoadThread = new QThread(this);
    pl->list = PluginsManager::getInstance(this)->pluginsList();;
    pl->connect(m_pluginLoadThread, &QThread::started, pl, &PluginLoader::runLoader);
    pl->connect(pl, SIGNAL(workFinished()), m_pluginLoadThread, SLOT(quit()));
    pl->connect(pl, &PluginLoader::pluginLoad, m_contentView, &ContentView::loadPluginInstance);
    pl->connect(m_pluginLoadThread, SIGNAL(finished()), pl, SLOT(deleteLater()));
    pl->connect(m_pluginLoadThread, SIGNAL(finished()), m_pluginLoadThread, SLOT(deleteLater()));
    pl->moveToThread(m_pluginLoadThread);
#endif
}

void Frame::loadContens()
{
#ifdef DCC_PRELOAD_MODULE_BACKEND
    m_pluginLoadThread->start();
    qDebug() << "plugin load thread" << m_pluginLoadThread << "started";
#endif

#ifdef DCC_PRELOAD_HOME_UI
    setCursor(Qt::ArrowCursor);
#else
    PluginsManager *pm = PluginsManager::getInstance(this);
    connect(pm, &PluginsManager::pluginLoaded, [this, pm] {
        if (pm->count() == m_homeScreen->count())
        {
            QTimer::singleShot(5000, this, [ = ] {
                setCursor(Qt::ArrowCursor);

                QString deepinMlockall = QProcessEnvironment::systemEnvironment().value("DEEPIN_MLOCKALL");
                if (deepinMlockall == "true") {
                    qWarning()<< "call mlockall(MCL_CURRENT) to avoid swap";
                    mlockall(MCL_CURRENT);
                }
            });
        }
    });
#endif
}

Frame::~Frame()
{
//    m_showAni->deleteLater();
//    m_hideAni->deleteLater();
}

void Frame::show(bool imme)
{
    qDebug() << "frame show";
    if (m_visible || m_hideAni->state() == QPropertyAnimation::Running) {
        return;
    }
    m_visible = true;

    if (imme) {
        m_centeralWarpper->move(0, 0);
    } else {
        int startX = m_hideInLeft ? -DCC::ControlCenterWidth : DCC::ControlCenterWidth;
        startX += m_hideInLeft ? -DCC::FrameShadowWidth : DCC::FrameShadowWidth;
        int endX = m_hideInLeft ? -DCC::FrameShadowWidth : 0;

        m_hideAni->stop();
        m_showAni->stop();
        m_showAni->setStartValue(QPoint(startX, 0));
        m_showAni->setEndValue(QPoint(endX, 0));
        m_showAni->start();
    }

    connect(m_dbusXMouseArea, &DBusXMouseArea::ButtonRelease, this, &Frame::globalMouseReleaseEvent);
    QDBusPendingReply<QString> reply = m_dbusXMouseArea->RegisterFullScreen();
    reply.waitForFinished();
    m_dbusFullScreenKey = reply.value();

    QFrame::show();
    setFocus();
    activateWindow();

    emit xChanged();
    emit visibleChanged(isVisible());
}

void Frame::hide(bool imme)
{
    if (!m_autoHide || !m_visible || m_showAni->state() == QPropertyAnimation::Running) {
        qDebug() << "not hide" << m_autoHide << m_visible << m_showAni->state();
        return;
    }
    m_visible = false;

    if (imme) {
        QFrame::hide();
    } else {
        int endX = m_hideInLeft ? -DCC::ControlCenterWidth : DCC::ControlCenterWidth;
        int startX = m_hideInLeft ? -DCC::FrameShadowWidth : 0;

        m_hideAni->stop();
        m_showAni->stop();
        m_hideAni->setEndValue(QPoint(endX, 0));
        m_hideAni->setStartValue(QPoint(startX, 0));
        m_hideAni->start();
    }

    qDebug() << "unregister: " << m_dbusFullScreenKey;
    m_dbusXMouseArea->UnregisterArea(m_dbusFullScreenKey);//waitForFinished();
    disconnect(m_dbusXMouseArea, &DBusXMouseArea::ButtonRelease, this, &Frame::globalMouseReleaseEvent);

    emit xChanged();
    emit visibleChanged(isVisible());
}

// private slots
void Frame::selectModule(const QString &pluginId)
{
    qDebug() << "select" << pluginId;

    // when module changed, clear old module settings
    m_autoHide = true;

    if (pluginId == "home") {
        m_homeScreen->show();
        m_contentView->hide();
    } else {
        m_homeScreen->hide();
        m_contentView->show();
        m_contentView->switchToModule(pluginId);
    }

    if (!m_visible) {
        show();
    }
}

bool Frame::isVisible() const
{
    return m_visible && QFrame::isVisible();
}

void Frame::globalMouseReleaseEvent(int button, int x, int y)
{
    Q_UNUSED(button);

    if (!rect().contains(x - this->x(), y - this->y())) {
        qDebug() << "hide by global btn release";
        hide();
    }
}

void Frame::hideAndShowAnotherSide()
{
    hide();
    m_hideInLeft = !m_hideInLeft;
    QTimer::singleShot(DCC::CommonAnimationDuration + 10, this, SLOT(hideAndShowAnotherSideFinish()));
}

void Frame::hideAndShowAnotherSideFinish()
{
    emit hideInLeftChanged(m_hideInLeft);
    show();
}

void Frame::showHelpDocument()
{
    QProcess *process = new QProcess;

    connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished), process, &QProcess::deleteLater);

    process->start("dman dde-control-center");
}

void Frame::hideEvent(QHideEvent *e)
{
    QFrame::hideEvent(e);

    emit visibleChanged(isVisible());
}

void Frame::showEvent(QShowEvent *e)
{
    QFrame::showEvent(e);

    emit visibleChanged(isVisible());
}

void Frame::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_F1:        showHelpDocument();     break;
#ifdef QT_DEBUG
    case Qt::Key_Escape:    qApp->quit();           break;
#endif
    default:;
    }

    QFrame::keyPressEvent(e);
}

int Frame::visibleFrameXPos()
{
    return pos().x() + m_centeralWarpper->pos().x() - DCC::FrameShadowWidth;
}

bool Frame::autoHide() const
{
    return m_autoHide;
}

void Frame::setHideInLeft(bool hideInLeft)
{
    if (m_hideInLeft == hideInLeft) {
        return;
    }

    m_hideInLeft = hideInLeft;

    emit hideInLeftChanged(hideInLeft);
}

void Frame::updateGeometry()
{
    const QRect primaryRect = m_displayInter->primaryRect();
    qDebug() << "updateGeometry: " << primaryRect;

    int posX;
    if (m_hideInLeft) {
        posX = primaryRect.left();
    } else {
        posX = primaryRect.right() - DCC::ControlCenterWidth - DCC::FrameShadowWidth + 1;
    }

    move(posX, primaryRect.y());
    setFixedHeight(primaryRect.height());
    m_centeralWarpper->setFixedHeight(primaryRect.height());
    m_centeralWidget->setFixedHeight(primaryRect.height());
    m_contentView->setFixedHeight(primaryRect.height());
    m_homeScreen->setFixedHeight(primaryRect.height());

    QFrame::updateGeometry();
}

void Frame::setAutoHide(bool autoHide)
{
    if (m_autoHide == autoHide) {
        return;
    }

    m_autoHide = autoHide;
    emit autoHideChanged(autoHide);
}

void Frame::toggle(bool inLeft)
{
    if (m_hideAni->state() == QPropertyAnimation::Running ||
            m_showAni->state() == QPropertyAnimation::Running) {
        return;
    }

    const bool lastState = isHideInLeft();

    if (lastState == inLeft) {
        isVisible() ? hide() : show();
    } else {
        hideAndShowAnotherSide();
    }
}
