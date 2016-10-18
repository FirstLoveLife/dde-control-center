/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <QDebug>
#include <QPluginLoader>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QtConcurrent>
#include <QThread>

#include "contentview.h"
#include "sidebar.h"
#include "constants.h"
#include "controlcenterproxy.h"

void SinglePluginLoader::load()
{
    qDebug() << QThread::currentThreadId() << "begin load file" << module.path;
    QPluginLoader *pluginLoader = new QPluginLoader();
    pluginLoader->setFileName(module.path);
    QObject *instance = pluginLoader->instance();
    instance->moveToThread(qApp->thread());
    qDebug() << QThread::currentThreadId() << "end load file" << module.path;
    emit pluginLoad(module.id, instance);
    emit workFinished();
}

void PluginLoader::runLoader()
{
    foreach(ModuleMetaData module, list) {
        qDebug() << "begin load file" << module.path;
        QPluginLoader *pluginLoader = new QPluginLoader(this);
        pluginLoader->setFileName(module.path);
        QObject *instance = pluginLoader->instance();
        qDebug() << "end load file" << module.path;
        instance->moveToThread(qApp->thread());
        emit pluginLoad(module.id, instance);
        QThread::msleep(100);
//        QThread *sl = new QThread;
//        SinglePluginLoader *spl = new  SinglePluginLoader;
//        spl->loader = this;
//        spl->module = module;
//        spl->moveToThread(sl);
//        connect(sl, &QThread::started, spl, &SinglePluginLoader::load);
//        connect(sl, &QThread::destroyed, spl, &SinglePluginLoader::deleteLater);
//        connect(sl, &QThread::destroyed, sl, &QThread::deleteLater);
//        connect(spl, &SinglePluginLoader::pluginLoad, this, &PluginLoader::pluginLoad);
//        spl->load();
//        sl->start();
//        QThread::msleep(100);
    }
}

ContentView::ContentView(ControlCenterProxy *proxy, QWidget *parent)
    : QFrame(parent),
      m_controlCenterProxy(proxy)
{
    m_pluginLoader = new QPluginLoader(this);
#ifdef QT_DEBUG
    m_pluginLoader->setLoadHints(QLibrary::ResolveAllSymbolsHint);
#endif
    m_pluginsManager = PluginsManager::getInstance(this);

    m_sideBar = new SideBar(parent);

    m_leftSeparator = new DSeparatorVertical;
    m_rightSeparator = new DSeparatorVertical;

    m_lastPluginWidgetContainer = new QWidget;
    m_lastPluginWidgetContainerLayout = new QHBoxLayout(m_lastPluginWidgetContainer);

    m_lastPluginWidgetContainerLayout->setMargin(0);
    m_lastPluginWidgetContainerLayout->setSpacing(0);

    m_layout = new QHBoxLayout(this);
    m_layout->addWidget(m_leftSeparator);
    m_layout->addWidget(m_sideBar);
    m_layout->addWidget(m_rightSeparator);
    m_layout->setSpacing(0);
    m_layout->setMargin(0);

    if (m_hideInLeft) {
        m_rightSeparator->hide();
//        m_sideBar->getTipFrame()->setArrowDirection(DTipsFrame::ArrowLeft);
        m_layout->insertWidget(0, m_lastPluginWidgetContainer);
    } else {
        m_leftSeparator->hide();
//        m_sideBar->getTipFrame()->setArrowDirection(DTipsFrame::ArrowRight);
        m_layout->addWidget(m_lastPluginWidgetContainer);
    }

    QFrame::hide();
    m_opacityEffect = new QGraphicsOpacityEffect;
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    m_showAni = new QPropertyAnimation(m_opacityEffect, "opacity");
    m_showAni->setDuration(DCC::CommonAnimationDuration);

    m_hideAni = new QPropertyAnimation(m_opacityEffect, "opacity");
    m_hideAni->setDuration(DCC::CommonAnimationDuration);

    connect(m_sideBar, &SideBar::moduleSelected, this, &ContentView::onModuleSelected);
    connect(m_hideAni, &QPropertyAnimation::finished, this, &QFrame::hide);
    connect(m_showAni, &QPropertyAnimation::finished, this, static_cast<void (ContentView::*)()>(&ContentView::update));
    connect(m_pluginsManager, &PluginsManager::showHome, this, &ContentView::switchToHome);

    setLayout(m_layout);
}

ContentView::~ContentView()
{
    m_pluginLoader->deleteLater();
    m_opacityEffect->deleteLater();
    m_showAni->deleteLater();
    m_hideAni->deleteLater();
}

void ContentView::switchToModule(ModuleMetaData module)
{
    qDebug() << "switchToModule" << module.id << module.path;
    if (m_lastPluginInterface && m_lastPluginWidget && m_lastPluginPath == module.path) {
        return;
    }
    m_lastPluginPath = module.path;

    unloadPlugin();

    // load new plugin
    m_sideBar->blockSignals(true);
    m_sideBar->switchToModule(module);
    m_sideBar->blockSignals(false);

    QWidget *content = nullptr;
#ifdef DCC_CACHE_MODULES
    if (m_pluginsCache.contains(module.path)) {
        content = m_pluginsCache.value(module.path, nullptr);
    } else {
        content = loadPluginNow(module);
    }
#else
    content = loadPlugin(module);
#endif
    if (!content) {
        qCritical() << module.path << "content not load";
        return;
    }
    m_lastPluginWidget = content;
    m_lastPluginWidget->setFixedWidth(DCC::ModuleContentWidth);
    m_lastPluginWidgetContainerLayout->addWidget(m_lastPluginWidget);
    m_lastPluginWidget->show();
}

void ContentView::hide()
{
    m_showAni->stop();
    m_hideAni->stop();
    m_hideAni->setStartValue(1.0);
    m_hideAni->setEndValue(0.0);
    m_hideAni->start();
}

void ContentView::show()
{
    m_hideAni->stop();
    m_showAni->stop();
    m_showAni->setStartValue(0.0);
    m_showAni->setEndValue(1.0);
    m_showAni->start();

    QFrame::show();
}

void ContentView::reLayout(bool hideInLeft)
{
    if (hideInLeft == m_hideInLeft) {
        return;
    }

    m_hideInLeft = hideInLeft;

    m_rightSeparator->setHidden(hideInLeft);
    m_leftSeparator->setVisible(hideInLeft);
    int index = hideInLeft ? 3 : 0;
    if (m_layout->count() > 3) {
        QLayoutItem *item = m_layout->takeAt(index);

        if (item) {
            m_layout->insertItem(3 - index, item);
        }
    }
}

void ContentView::switchToHome()
{
    emit backToHome();
}

void ContentView::loadPluginInstance(const QString &id, QObject *instance)
{
#ifdef DCC_CACHE_MODULES
    ModuleMetaData module = m_pluginsManager->pluginMetaData(id);
    if (!m_pluginsCache.contains(module.path)) {
        qDebug() << "main thread begin load" << module.id << instance;
        m_moduleCache[module.path] = instance;
    #ifdef DCC_PRELOAD_MODULE_UI
        if (module.id != "display")
            m_pluginsCache[module.path] = loadPlugin(module);
        emit m_pluginsManager->pluginLoaded(module);
    #endif
    }
#else
    Q_UNUSED(id);
    Q_UNUSED(instance);
#endif
}


void ContentView::switchToModule(const QString pluginId)
{
    switchToModule(m_pluginsManager->pluginMetaData(pluginId));
}

QWidget *ContentView::loadModuleContent()
{
    QWidget *content = NULL;
    QPair<ModuleMetaData, ModuleInterface *> moduleInfo = m_moduleLoadQueue.takeFirst();
    ModuleMetaData module = moduleInfo.first;
    ModuleInterface *m_interface = moduleInfo.second;

    qDebug() << "loadModuleContent" << module.id << "begin";

    if (!m_interface) {
        return content;
    }
    m_lastPluginInterface = m_interface;
    content = m_interface->getContent();

    if (!content) {
        qDebug() << "content is null !!" << module.path;
        // display error infomation
        const QString error = m_pluginLoader->errorString();
        // this label will destory when call unloadPlugin() next time
        QLabel *errorLabel = new QLabel(error);
        errorLabel->setWordWrap(true);
        errorLabel->setStyleSheet("color:red;");
        content = errorLabel;
        return content;
    }
    m_lastPluginInterface->setProxy(m_controlCenterProxy);
    qDebug() << "loadModuleContent" << module.id << "end" << content;

#ifdef DCC_CACHE_MODULES
    m_pluginsCache[module.path] = content;
#endif
    return content;
}

QWidget *ContentView::loadPluginNow(ModuleMetaData module)
{
    QPluginLoader *pluginLoader = new QPluginLoader(this);
    pluginLoader->setFileName(module.path);
    QObject *instance = pluginLoader->instance();
#ifdef DCC_CACHE_MODULES
    m_moduleCache[module.path] = instance;
#endif
    ModuleInterface *interface = qobject_cast<ModuleInterface *>(instance);
    m_moduleLoadQueue.push_back(QPair<ModuleMetaData, ModuleInterface *>(module, interface));
    QWidget *content = loadModuleContent();

#ifdef DCC_CACHE_MODULES
    m_pluginsCache[module.path] = content;
#endif
    return content;
}

QWidget *ContentView::loadPlugin(ModuleMetaData module)
{
    qDebug() << "loadPlugin start";

#ifdef DCC_CACHE_MODULES
#ifndef DISABLE_LAZYLOAD_MODULE
    QPluginLoader *pluginLoader = new QPluginLoader(this);
    pluginLoader->setFileName(module.path);
    m_moduleCache[module.path] = pluginLoader->instance();
#endif
    if (!m_pluginsCache.contains(module.path))
    {
        QPluginLoader *pluginLoader = new QPluginLoader(this);
        pluginLoader->setFileName(module.path);
        m_moduleCache[module.path] = pluginLoader->instance();
    }
    QObject *instance = m_moduleCache[module.path];
#else
    m_pluginLoader->setFileName(module.path);
    QObject *instance = m_pluginLoader->instance();
#endif

    ModuleInterface *interface = qobject_cast<ModuleInterface *>(instance);
    m_moduleLoadQueue.push_back(QPair<ModuleMetaData, ModuleInterface *>(module, interface));
    QWidget *content = NULL;

#ifdef DCC_PRELOAD_MODULE_UI
    QTimer::singleShot(800, this, &ContentView::loadModuleContent);
#else
    content = loadModuleContent();
#endif

    return content;
}

void ContentView::onModuleSelected(ModuleMetaData meta)
{
    qDebug() << meta.id;

    if (meta.id == "home") {
        // when goto home screen, notify plugin know.
        if (m_lastPluginInterface) {
            m_lastPluginInterface->preUnload();
        }

        emit backToHome();
        return;
    }

    if (meta.id == "shutdown") {
        emit shutdownSelected();
        return;
    }

#ifdef DCC_DROP_SWITCH_REQUEST
    // prevent the UI from blocking, we choose to drop some
    // module-switch requests on slower machines.
    static QTimer *timer = nullptr;
    if (!timer) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(300);
    }

    timer->disconnect();

    if (timer->isActive()) {
        timer->stop();
        connect(timer, &QTimer::timeout, [this, meta] {
            switchToModule(meta);
        });
    } else {
        switchToModule(meta);
    }

    timer->start();
#else
    // switch to another plugin
    switchToModule(meta);
#endif
}

void ContentView::unloadPlugin()
{
    if (m_lastPluginInterface) {
        m_lastPluginInterface->preUnload();
    }

    if (m_lastPluginWidget) {
        m_lastPluginWidget->hide();
        m_lastPluginWidget->setParent(nullptr);
#ifndef DCC_CACHE_MODULES
        m_lastPluginWidget->deleteLater();
#endif
//        delete m_lastPluginWidget;
        m_lastPluginWidget = nullptr;
    }

    if (m_lastPluginInterface) {
//        m_lastPluginInterface->preUnload();
#ifndef DCC_CACHE_MODULES
        delete m_lastPluginInterface;
#endif
        m_lastPluginInterface = nullptr;
    }
}
