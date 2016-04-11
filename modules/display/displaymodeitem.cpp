/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <QVBoxLayout>
#include <QPainter>
#include <QLabel>
#include <QDebug>

#include <libdui/libdui_global.h>
#include <libdui/dthememanager.h>
#include <libdui/dseparatorhorizontal.h>

#include "../widgets/imagenamebutton.h"
#include "displaymodeitem.h"

DUI_USE_NAMESPACE

DisplayModeItem::DisplayModeItem(bool showSeparator, bool showTitle, bool showRightArrow, QWidget *parent) :
    QLabel(parent),
    m_title(new QLabel),
    m_text(new QLabel),
    m_checked(false),
    m_hover(false),
    m_clickCheck(true)
{
    m_title->setObjectName("Title");
    m_text->setObjectName("Text");
    m_text->setWordWrap(showTitle);

    D_THEME_INIT_WIDGET(DisplayModeItem, hover, checked);

    if(showTitle){
        QHBoxLayout *mainLayout = new QHBoxLayout;

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addSpacing(10);
        layout->addWidget(m_title);
        layout->addSpacing(5);
        layout->addWidget(m_text);
        layout->addStretch(1);
        if(showSeparator){
            layout->addSpacing(10);
            layout->addWidget(new DSeparatorHorizontal);
        }
        layout->setMargin(0);
        layout->setSpacing(0);

        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(60, 0, 0, 0);

        mainLayout->addLayout(layout, 1);

        if (showRightArrow) {
            ImageNameButton * button = new ImageNameButton("arrow_right", this);
            mainLayout->addWidget(button);

            connect(button, &ImageNameButton::clicked, this, &DisplayModeItem::rightArrowClicked);
        }

        setLayout(mainLayout);
    }else{
        m_text->setParent(this);
        m_text->move(30, 2);
    }
}

DisplayModeItem::~DisplayModeItem()
{
    if(m_checked)
        activeItem = NULL;
    m_title->deleteLater();
}

QString DisplayModeItem::iconName() const
{
    return m_iconName;
}

QString DisplayModeItem::title() const
{
    return m_title->text();
}

QString DisplayModeItem::text() const
{
    return m_text->text();
}

bool DisplayModeItem::checked() const
{
    return m_checked;
}

bool DisplayModeItem::hover() const
{
    return m_hover;
}

bool DisplayModeItem::clickCheck() const
{
    return m_clickCheck;
}

QString DisplayModeItem::iconText() const
{
    return m_iconText;
}

void DisplayModeItem::setIconName(QString iconName)
{
    if(m_iconName == iconName)
        return;

    m_iconName = iconName;

    updateIcon();
}

void DisplayModeItem::setTitle(QString title)
{
    m_title->setText(title);
}

void DisplayModeItem::setText(QString text)
{
    m_text->setText(text);
    setFixedWidth(30 + m_text->sizeHint().width());
}

DisplayModeItem *DisplayModeItem::activeItem = NULL;

void DisplayModeItem::setChecked(bool active)
{
    if(m_checked == active)
        return;

    m_checked = active;

    updateIcon();

    if(active){
        if(activeItem)
            activeItem->setChecked(false);
        activeItem = this;
    }else{
        activeItem = NULL;
    }

    emit checkedChanged(active);
}

void DisplayModeItem::setClickCheck(bool clickCheck)
{
    m_clickCheck = clickCheck;
}

void DisplayModeItem::setIconText(QString iconText)
{
    m_iconText = iconText;
}

void DisplayModeItem::enterEvent(QEvent *e)
{
    QLabel::enterEvent(e);

    setHover(true);
}

void DisplayModeItem::leaveEvent(QEvent *e)
{
    QLabel::leaveEvent(e);

    setHover(false);
}

void DisplayModeItem::mouseReleaseEvent(QMouseEvent *e)
{
    QLabel::mouseReleaseEvent(e);

    if(m_clickCheck && !m_checked){
        setChecked(true);
    }

    emit clicked();
}

void DisplayModeItem::paintEvent(QPaintEvent *e)
{
    QLabel::paintEvent(e);

    QPainter pa(this);
    QTextOption tmpOption;
    tmpOption.setAlignment(Qt::AlignVCenter);
    QRect rect = this->rect();
    rect.setLeft(30);
    pa.drawText(rect, m_iconText, tmpOption);
    pa.end();
}

void DisplayModeItem::setHover(bool arg)
{
    if(m_hover == arg)
        return;

    m_hover = arg;

    updateIcon();

    emit hoverChanged(arg);
}

void DisplayModeItem::updateIcon()
{
    QString icon_path = ":/";
    icon_path += DThemeManager::instance()->theme().append("/images/").append(m_iconName);

    if(m_checked){
        setPixmap(QPixmap(QString("%1_active.png").arg(icon_path)));
    }else{
        if(m_hover){
            setPixmap(QPixmap(QString("%1_hover.png").arg(icon_path)));
        }else{
            setPixmap(QPixmap(QString("%1_normal.png").arg(icon_path)));
        }
    }
}
