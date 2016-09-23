/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>

#include <dbuttonlist.h>
#include <dexpandgroup.h>
#include <dseparatorhorizontal.h>
#include <ddialog.h>

#include "moduleheader.h"
#include "normallabel.h"
#include "genericlistitem.h"

#include "monitor.h"
#include "monitorground.h"
#include "customsettings.h"
#include "titleandwidget.h"

DWIDGET_USE_NAMESPACE

CustomSettings::CustomSettings(DisplayInterface *dbusDisplay, MonitorGround *monitorGround,
                               const QList<MonitorInterface *> &list,
                               QWidget *parent)
    : QFrame(parent),
      m_dbusMonitors(list),
      m_dbusDisplay(dbusDisplay),
      m_monitorGround(monitorGround)
{
    m_rotationMap[1] = tr("Normal");
    m_rotationMap[8] = tr("Rotate right");
    m_rotationMap[4] = tr("Upside down");
    m_rotationMap[2] = tr("Rotate left");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
}

CustomSettings::~CustomSettings()
{
    if (m_dbusDisplay->hasChanged()) {
        m_dbusDisplay->ResetChanges();
    }
}

void CustomSettings::updateUI(const QList<MonitorInterface *> &list)
{
    for (int i = 0; i < m_mainLayout->count();) {
        QLayoutItem *item = m_mainLayout->itemAt(i);
        QWidget *w = item->widget();
        if (w) {
            w->deleteLater();
        }
        m_mainLayout->removeItem(item);
        delete item;
    }

    m_dbusMonitors = list;

    m_monitorNameList = m_dbusDisplay->ListOutputNames();

    DExpandGroup *expandGroup = new DExpandGroup(this);

    m_brightnessHeaderLine = new DBaseLine;
    NormalLabel *brightnessTitle = new NormalLabel(tr("Brightness"));
    m_brightnessHeaderLine->setLeftContent(brightnessTitle);
    m_brightnessLineSeparator = new DSeparatorHorizontal;

    ListWidget *brightnessList = new ListWidget;
    m_brightnessExpand = new DArrowLineExpand;
    brightnessList->setItemSize(290, 0);
    m_brightnessExpand->setTitle(tr("Brightness"));
    m_brightnessExpand->setContent(brightnessList);
    expandGroup->addExpand(m_brightnessExpand);

    connect(brightnessList, &ListWidget::countChanged, [brightnessList] {
        brightnessList->setFixedHeight(brightnessList->height() - 2);
    });

    if (m_dbusMonitors.count() > 0) {
        DArrowLineExpand *enableMonitor = new DArrowLineExpand;
        enableMonitor->setTitle(tr("Enable Monitor"));
        expandGroup->addExpand(enableMonitor);

        m_enableMonitorList = new ListWidget(ListWidget::MultipleCheck);
        m_enableMonitorList->setItemSize(310, 25);
        m_enableMonitorList->setCheckable(true);

        connect(m_enableMonitorList, &ListWidget::checkedChanged, [this](int index, bool arg) {
            m_dbusMonitors[index]->SwitchOn(arg).waitForFinished();
            m_enableMonitorList->setEnableUncheck(m_enableMonitorList->checkedList().count() > 1);
            m_monitorGround->adjustMonitorPosition();
        });

        TitleAndWidget *enableTitle = new TitleAndWidget(m_enableMonitorList, false);
        enableTitle->setText(tr("Please select the monitor you want to enable (checkable)"));
        enableTitle->setFixedSize(290, m_dbusMonitors.count() * 25 + 30);
        enableMonitor->setContent(enableTitle);

        m_primaryMonitor = new DArrowLineExpand;
        m_primaryMonitor->setTitle(tr("Primary"));
        expandGroup->addExpand(m_primaryMonitor);

        m_primaryMonitorList = new DButtonList;
        m_primaryMonitorList->setItemWidth(310);
        m_primaryMonitor->setContent(m_primaryMonitorList);

        connect(m_dbusDisplay, &DisplayInterface::PrimaryChanged, m_primaryMonitorList, [this] {
            QString str = m_dbusDisplay->primary();
            int checkedIndex = -1;
            for (int i = 0; i < m_primaryMonitorList->count(); ++i)
            {
                IconButton *button = m_primaryMonitorList->getButtonByIndex(i);
                if (button && button->text() == str) {
                    checkedIndex = i;
                }
            }
            if (checkedIndex != -1) {
                m_primaryMonitorList->blockSignals(true);
                m_primaryMonitorList->checkButtonByIndex(checkedIndex);
                m_primaryMonitorList->blockSignals(false);
            }
        }, Qt::DirectConnection);

        connect(m_primaryMonitorList, &DButtonList::buttonChecked, m_dbusDisplay, &DisplayInterface::SetPrimary);

        m_mainLayout->addWidget(enableMonitor);
        m_mainLayout->addWidget(m_primaryMonitor);

        enableMonitor->setVisible(m_dbusMonitors.count() > 1);
    }

    DArrowLineExpand *resolutionExpand = new DArrowLineExpand;
    resolutionExpand->setTitle(tr("Resolution"));
    expandGroup->addExpand(resolutionExpand);

    DArrowLineExpand *rotationExpand = new DArrowLineExpand;
    rotationExpand->setTitle(tr("Rotation"));
    expandGroup->addExpand(rotationExpand);

    ListWidget *resolutionList = new ListWidget;
    resolutionList->setFixedWidth(290);

    connect(resolutionList, &ListWidget::countChanged, [resolutionList] {
        resolutionList->setFixedHeight(resolutionList->height() - 2);
    });

    ListWidget *rotationList = new ListWidget;
    rotationList->setFixedWidth(290);

    connect(rotationList, &ListWidget::countChanged, [rotationList] {
        rotationList->setFixedHeight(rotationList->height() - 2);
    });

    foreach(const QString & name, m_monitorNameList) {
        TitleAndWidget *widget = new TitleAndWidget(getBrightnessSlider(name));
        widget->setText(tr("Monitor %1").arg(name));
        widget->setFixedHeight(50);
        brightnessList->addWidget(widget);
    }

    DSlider *tmp_slider = getBrightnessSlider(m_dbusDisplay->primary());
    tmp_slider->setFixedWidth(180);
    m_brightnessHeaderLine->setRightContent(tmp_slider);
    m_brightnessHeaderLine->setFixedHeight(30);

    int currentPrimaryIndex = -1;
    for (int i = 0; i < m_dbusMonitors.count(); ++i) {
        MonitorInterface *dbusMonitor = m_dbusMonitors[i];

        if (m_enableMonitorList) {
            GenericListItem *enableMonitorItem = new GenericListItem;
            enableMonitorItem->setTitle(dbusMonitor->name());
            m_enableMonitorList->addWidget(enableMonitorItem, Qt::AlignLeft);

            m_primaryMonitorList->addButton(dbusMonitor->name());
            if (dbusMonitor->name() == m_dbusDisplay->primary()) {
                currentPrimaryIndex = i;
            }
            m_primaryMonitorList->setFixedHeight(m_primaryMonitorList->count()*MENU_ITEM_HEIGHT);
        }

        DButtonGrid *resolutionButtons = new DButtonGrid(0, 3);
        resolutionButtons->setItemSize(90, 30);
        updateResolutionButtons(dbusMonitor, resolutionButtons);

        TitleAndWidget *titleWidget_resolution = new TitleAndWidget(resolutionButtons);
        titleWidget_resolution->setText(tr("Monitor %1").arg(dbusMonitor->name()));
        titleWidget_resolution->setFixedSize(290, resolutionButtons->rowCount() * 30 + 30);
        resolutionList->addWidget(titleWidget_resolution);
        resolutionExpand->setContent(resolutionList);

        connect(resolutionList, &ListWidget::visibleCountChanged,
        titleWidget_resolution, [titleWidget_resolution](int count) {
            titleWidget_resolution->setTitleVisible(count > 1);
        }, Qt::DirectConnection);
        titleWidget_resolution->setTitleVisible(resolutionList->visibleCount() > 1);

        DButtonGrid *rotationButtons = new DButtonGrid;
        rotationButtons->setItemSize(90, 30);
        QStringList rotations = getRotationLabels(dbusMonitor);
        rotationButtons->setColumnCount(2);
        rotationButtons->setRowCount(qCeil(rotations.length() / 2.0));
        rotationButtons->addButtons(rotations);

        TitleAndWidget *titleWidget_rotation = new TitleAndWidget(rotationButtons);
        titleWidget_rotation->setText(tr("Monitor %1").arg(dbusMonitor->name()));
        titleWidget_rotation->setFixedSize(290, rotationButtons->rowCount() * 30 + 30);
        rotationList->addWidget(titleWidget_rotation);
        rotationExpand->setContent(rotationList);

        connect(rotationList, &ListWidget::visibleCountChanged,
        titleWidget_rotation, [titleWidget_rotation](int count) {
            titleWidget_rotation->setTitleVisible(count > 1);
        }, Qt::DirectConnection);
        titleWidget_rotation->setTitleVisible(rotationList->visibleCount() > 1);

        updateRotationButtons(dbusMonitor, rotationButtons);

        if (dbusMonitor->opened()) {
            if (m_enableMonitorList) {
                m_enableMonitorList->setChecked(i, true);
            }
        } else {
            resolutionList->hideWidget(i);
            rotationList->hideWidget(i);
        }

        connect(dbusMonitor, &MonitorInterface::OpenedChanged,
                titleWidget_resolution,
                [this, titleWidget_resolution, dbusMonitor, resolutionList,
        rotationList, titleWidget_rotation, i] {
            if (dbusMonitor->opened())
            {
                resolutionList->showWidget(resolutionList->indexOf(titleWidget_resolution));
                rotationList->showWidget(rotationList->indexOf(titleWidget_rotation));
            } else{
                resolutionList->hideWidget(resolutionList->indexOf(titleWidget_resolution));
                rotationList->hideWidget(rotationList->indexOf(titleWidget_rotation));
            }
            updateBrightnessLayout();
            if (m_enableMonitorList)
            {
                m_enableMonitorList->setChecked(i, dbusMonitor->opened());
                m_enableMonitorList->setEnableUncheck(m_enableMonitorList->checkedList().count() > 1);
            }
        }, Qt::DirectConnection);

        connect(dbusMonitor, &MonitorInterface::IsCompositedChanged,
                this, &CustomSettings::updateBrightnessLayout, Qt::DirectConnection);

        connect(dbusMonitor, &MonitorInterface::CurrentModeChanged,
        resolutionButtons, [resolutionButtons, dbusMonitor] {
            MonitorMode currentMode = dbusMonitor->currentMode();
            QString currentResolution = QString("%1x%2").arg(currentMode.width).arg(currentMode.height);
            resolutionButtons->checkButtonByText(currentResolution);
        }, Qt::DirectConnection);
        connect(resolutionButtons, &DButtonGrid::buttonCheckedIndexChanged,
        this, [dbusMonitor](int index) {
            QDBusPendingReply<MonitorModeList> modesReply = dbusMonitor->ListModes();
            modesReply.waitForFinished();
            MonitorModeList monitorModeList = modesReply.value();
            if (index >= 0 && index < monitorModeList.count()) {
                uint id = monitorModeList.at(index).id;
                if (dbusMonitor->currentMode().id != id) {
                    dbusMonitor->SetMode(id);
                }
            }
        }, Qt::DirectConnection);

        connect(dbusMonitor, &MonitorInterface::RotationChanged,
        rotationButtons, [this, rotationButtons, dbusMonitor] {
            ushort currentRotation = dbusMonitor->rotation();
            rotationButtons->checkButtonByText(m_rotationMap[currentRotation]);
        }, Qt::DirectConnection);
        connect(rotationButtons, &DButtonGrid::buttonCheckedIndexChanged,
        this, [dbusMonitor, rotations, this](int index) {
            foreach(ushort r, m_rotationMap.keys()) {
                if (m_rotationMap.value(r) == rotations.at(index)) {
                    dbusMonitor->SetRotation(r);
                    break;
                }
            }
        }, Qt::DirectConnection);
    }
    if (currentPrimaryIndex != -1)
        m_primaryMonitorList->checkButtonByIndex(currentPrimaryIndex);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    if (m_monitorNameList.count() == 1) {
        m_cancelButton->setVisible(m_dbusDisplay->hasChanged());
        m_applyButton->setVisible(m_cancelButton->isVisible());

        connect(m_dbusDisplay, &DisplayInterface::HasChangedChanged,
                this, &CustomSettings::onHasChangedChanged);
    } else {
        m_cancelButton->show();
        m_applyButton->show();

        disconnect(m_dbusDisplay, &DisplayInterface::HasChangedChanged,
                   this, &CustomSettings::onHasChangedChanged);
    }

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addSpacing(5);
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addSpacing(10);

    connect(m_monitorGround, &MonitorGround::editingChanged, buttonLayout, [this] {
        if (m_dbusDisplay->hasChanged() || m_monitorGround->editing())
            m_applyButton->setText(tr("Apply"));
        else
        { m_applyButton->setText(tr("Confirm")); }
    });
    connect(m_cancelButton, &DTextButton::clicked, buttonLayout, [this] {
        m_monitorGround->cancelEdit();

        if (m_dbusDisplay->hasChanged())
        {
            m_dbusDisplay->ResetChanges();
        }

        emit cancel();
    });
    connect(m_applyButton, &DTextButton::clicked, buttonLayout, [this] {
        m_monitorGround->applyEdit();

        if (m_dbusDisplay->hasChanged()) {
            DDialog dialog;
            int time_left = 30;

            dialog.setWindowFlags(window()->windowFlags());
            dialog.setTitle(tr("Do you want to keep these display settings?"));
            dialog.setMessage(QString(tr("Reverting to previous display settings in "
            "<font color='white'>%1</font> seconds.")).arg(--time_left));
            dialog.addButton(tr("Revert"));
            dialog.addButton(tr("Keep Changes"));

            QTimer timer;

            connect(&timer, &QTimer::timeout, this, [&dialog, &time_left, &timer] {
                dialog.setMessage(QString(tr("Reverting to previous display settings in "
                "<font color='white'>%1</font> seconds.")).arg(--time_left));
                if (time_left <= 0)
                {
                    timer.stop();
                    dialog.done(0);
                }
            });

            const QScreen *screen_dialog = nullptr;
            const QScreen *screen_mouse = nullptr;

            for (const QScreen *screen : QGuiApplication::screens()) {
                if (screen->geometry().contains(window()->pos())) {
                    screen_dialog = screen;
                    break;
                } else if (screen->geometry().contains(QCursor::pos())) {
                    screen_mouse = screen;
                }
            }

            const QScreen *target_screen = screen_dialog ? screen_dialog : screen_mouse;

            connect(target_screen, &QScreen::geometryChanged, &dialog, [&dialog](const QRect & rect) {
                dialog.moveToCenterByRect(rect);
            });

            m_dbusDisplay->Apply();

            timer.start(1000);

            window()->setProperty("autoHide", false);

            int button_index = dialog.exec();

            window()->setProperty("autoHide", true);

            if (button_index == 1) {
                m_dbusDisplay->SaveChanges();
            } else {
                m_dbusDisplay->ResetChanges();
            }
        }

        emit cancel();
    });

    m_mainLayout->addWidget(resolutionExpand);
    m_mainLayout->addWidget(rotationExpand);
    m_mainLayout->addWidget(m_brightnessHeaderLine);
    m_mainLayout->addWidget(m_brightnessLineSeparator);
    m_mainLayout->addWidget(m_brightnessExpand);
    m_mainLayout->addSpacing(10);
    m_mainLayout->addLayout(buttonLayout);
    m_mainLayout->addStretch(1);

    updateBrightnessLayout();
}

void CustomSettings::updateResolutionButtons(MonitorInterface *dbus, DButtonGrid *resolutionButtons)
{
    QStringList resolutions = getResolutionLabels(dbus);

    MonitorMode currentMode = dbus->currentMode();
    QString currentResolution = QString("%1x%2").arg(currentMode.width).arg(currentMode.height);
    int index = resolutions.indexOf(currentResolution);
    resolutionButtons->clear();
    resolutionButtons->addButtons(resolutions);

    if (index >= 0) {
        resolutionButtons->checkButtonByIndex(index);
    }
}

void CustomSettings::updateRotationButtons(MonitorInterface *dbus, DButtonGrid *rotationButtons)
{
    QStringList rotations = getRotationLabels(dbus);

    ushort currentRotation = dbus->rotation();
    int index = rotations.indexOf(m_rotationMap[currentRotation]);

    if (index >= 0) {
        rotationButtons->checkButtonByIndex(index);
    }
}

void CustomSettings::updateBrightnessSlider(const QString &name, DSlider *brightnessSlider)
{
    BrightnessMap brightnessMap = m_dbusDisplay->brightness();
    m_ignoreSliderChang = true;
    brightnessSlider->setValue(brightnessMap[name] * 10);
    m_ignoreSliderChang = false;
}

void CustomSettings::updateBrightnessLayout()
{
    int openedMonitorCount = 0;
    foreach(MonitorInterface * dbus, m_dbusMonitors) {
        if (dbus->opened()) {
            if (dbus->isComposited()) {
                openedMonitorCount += 2;
            } else {
                openedMonitorCount ++;
            }
        }
    }

    if (openedMonitorCount < 2) {
        if (m_primaryMonitor) {
            m_primaryMonitor->hide();
        }
        m_brightnessExpand->hide();
        m_brightnessHeaderLine->show();
        m_brightnessLineSeparator->show();
    } else {
        m_brightnessHeaderLine->hide();
        m_brightnessLineSeparator->hide();
        m_brightnessExpand->show();
        if (m_primaryMonitor) {
            m_primaryMonitor->show();
        }
    }
}

void CustomSettings::onHasChangedChanged()
{
    QThread::msleep(300);
    if (m_dbusDisplay->hasChanged() || m_monitorGround->editing()) {
        m_applyButton->setText(tr("Apply"));
    } else {
        m_applyButton->setText(tr("Confirm"));
    }

    if (m_monitorNameList.count() == 1) {
        m_cancelButton->setVisible(m_dbusDisplay->hasChanged());
        m_applyButton->setVisible(m_cancelButton->isVisible());
    }
}

QStringList CustomSettings::getResolutionLabels(MonitorInterface *dbus)
{
    QStringList resolutions;

    QDBusPendingReply<MonitorModeList> modesReply = dbus->ListModes();
    modesReply.waitForFinished();
    MonitorModeList monitorModeList = modesReply.value();

    qDebug() << "-----------------getResolutionLabels start--------------------------";
    qDebug() << "monitor name:" << dbus->name();

    if (monitorModeList.isEmpty()) {
        qDebug() << "monitorModeList is empty.";
        qDebug() << m_mapNameToResolutionLabels;
        return m_mapNameToResolutionLabels[dbus->name()];
    }

    foreach(MonitorMode mode, monitorModeList) {
        if (qMax(mode.width, mode.height) >= 800) {
            resolutions << QString("%1x%2").arg(mode.width).arg(mode.height);
        }
    }

    m_mapNameToResolutionLabels[dbus->name()] = resolutions;

    qDebug() << resolutions << m_mapNameToResolutionLabels;
    qDebug() << "-----------------getResolutionLabels end--------------------------";

    return resolutions;
}

QStringList CustomSettings::getRotationLabels(MonitorInterface *dbus)
{
    QStringList rotations;

    QDBusPendingReply<UshortList> rotationsReply = dbus->ListRotations();
    rotationsReply.waitForFinished();
    UshortList monitorRotations = rotationsReply.value();

    qDebug() << "-----------------getRotationLabels start--------------------------";
    qDebug() << "monitor name:" << dbus->name();

    if (monitorRotations.isEmpty()) {
        qDebug() << "monitorRotations is empty.";
        qDebug() << m_mapNameToRotationLabels;
        return m_mapNameToRotationLabels[dbus->name()];
    }

    foreach(ushort rotationId, monitorRotations) {
        rotations << m_rotationMap[rotationId];
    }

    m_mapNameToRotationLabels[dbus->name()] = rotations;

    qDebug() << monitorRotations << rotations;
    qDebug() << "-----------------getRotationLabels end--------------------------";

    return rotations;
}

DSlider *CustomSettings::getBrightnessSlider(const QString &name)
{
    DSlider *brightnessSlider = new DSlider(Qt::Horizontal);
    brightnessSlider->setSingleStep(1);
    brightnessSlider->setPageStep(1);
    // The monitor packed with loongson currently doesn't support backlight adjustion.
    // And the fallback mechanism doesn't have a good effect, either. So I just work
    // around this situation by raising the minimum brightness value.
#ifdef ARCH_MIPSEL
    brightnessSlider->setRange(3, 10);
#else
    brightnessSlider->setRange(1, 10);
#endif
    brightnessSlider->setMinimumWidth(290);

    updateBrightnessSlider(name, brightnessSlider);

    connect(brightnessSlider, &DSlider::valueChanged, this, [this, name](int value) {
        if (!m_ignoreSliderChang) {
            m_dbusDisplay->SetBrightness(name, value / 10.0);
        }
    }, Qt::DirectConnection);
    connect(m_dbusDisplay, &DisplayInterface::BrightnessChanged,
    brightnessSlider, [this, name, brightnessSlider] {
        updateBrightnessSlider(name, brightnessSlider);
    }, Qt::DirectConnection);

    return brightnessSlider;
}
