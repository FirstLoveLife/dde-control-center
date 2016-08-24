/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "createuserpanel.h"
#include "libintl.h"

CreateUserPanel::CreateUserPanel(QWidget *parent) : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    initDbusData();
    if (m_account && m_account->isValid()){
        initHeader();
        initInputLline();
        initConfirmLine();
        initInfoLine();

        m_layout->addStretch();
    }

    installEventFilter(this);
}

void CreateUserPanel::preDestroy()
{
    if (m_nameLine)
        m_nameLine->hideWarning();
    if (m_passwdNew)
        m_passwdNew->hideWarning();
    if (m_passwdRepeat)
        m_passwdRepeat->hideWarning();
}

bool CreateUserPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::HideToParent) {
        m_nameLine->hideWarning();
        m_passwdNew->hideWarning();
        m_passwdRepeat->hideWarning();
    }

    return QWidget::eventFilter(obj, event);
}

void CreateUserPanel::initDbusData()
{
    m_account = new DBusAccount(this);
    connect(m_account, &DBusAccount::UserAdded, this, &CreateUserPanel::onUserAdded);
    if (m_account->isValid())
        m_randIcon = m_account->RandUserIcon().value();
}

void CreateUserPanel::initHeader()
{
    QLabel *headerLabel = new QLabel(tr("Add User"));
    headerLabel->setObjectName("CreateHeaderLabel");
    headerLabel->setFixedHeight(DTK_WIDGET_NAMESPACE::EXPAND_HEADER_HEIGHT);
    headerLabel->setContentsMargins(DTK_WIDGET_NAMESPACE::HEADER_LEFT_MARGIN, 0 , 0, 0);
    headerLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_layout->addWidget(headerLabel);
    DSeparatorHorizontal *s = new DSeparatorHorizontal;
    m_layout->addWidget(s);
}

void CreateUserPanel::initInfoLine()
{
    QLabel *infoFrame = new QLabel;
    infoFrame->setObjectName("CreateInfoLabel");
    infoFrame->setFixedHeight(100);
    QHBoxLayout *hLayout = new QHBoxLayout(infoFrame);
    hLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);
    QVBoxLayout *vLayout  = new QVBoxLayout;
    vLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);

    m_avatar = new UserAvatar;
    m_avatar->setAvatarSize(UserAvatar::AvatarSmallSize);
    m_avatar->setFixedSize(ICON_SIZE, ICON_SIZE);
    m_avatar->setIcon(m_randIcon);

    m_newNameLabel = new QLabel(tr("new user"));
    m_newNameLabel->setObjectName("NewNameLabel");
    m_newNameLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    vLayout->addWidget(m_newNameLabel);

    hLayout->addSpacing(DTK_WIDGET_NAMESPACE::HEADER_LEFT_MARGIN);
    hLayout->addWidget(m_avatar);
    hLayout->addSpacing(DTK_WIDGET_NAMESPACE::HEADER_LEFT_MARGIN);
    hLayout->addLayout(vLayout);
    hLayout->addStretch(1);

    DSeparatorHorizontal *s = new DSeparatorHorizontal();
    m_layout->insertWidget(2, s);
    m_layout->insertWidget(2, infoFrame);
}

void CreateUserPanel::initInputLline()
{
    m_nameLine = new AccountInputLine();
    m_passwdNew = new AccountPasswdLine();
    m_passwdRepeat = new AccountPasswdLine();
    m_autoLogin = new AccountSwitchLine();
    connect(m_nameLine, &AccountInputLine::textChanged, this, &CreateUserPanel::onNameChanged);
    connect(m_passwdNew, &AccountPasswdLine::textChanged, this, &CreateUserPanel::onPasswdChanged);
    connect(m_passwdRepeat, &AccountPasswdLine::textChanged, this, &CreateUserPanel::onPasswdRepeatChanged);
    connect(m_nameLine, &AccountInputLine::focusChanged, this, &CreateUserPanel::onNameFocusChanged);
    connect(m_passwdNew, &AccountPasswdLine::focusChanged, this, &CreateUserPanel::onPasswdFocusChanged);
    connect(m_passwdRepeat, &AccountPasswdLine::focusChanged, this, &CreateUserPanel::onPasswdRepeatFocusChanged);

    QFont f = m_nameLine->lineEdit()->font();
    f.setCapitalization(QFont::AllLowercase);
    m_nameLine->lineEdit()->setFont(f);

    m_nameLine->setTitle(tr("Username"));
    m_passwdNew->setTitle(tr("Password"));
    m_passwdRepeat->setTitle(tr("Repeat Password"));
    m_autoLogin->setTitle(tr("Auto-login"));

    DSeparatorHorizontal *s1 = new DSeparatorHorizontal();
    DSeparatorHorizontal *s2 = new DSeparatorHorizontal();
    DSeparatorHorizontal *s3 = new DSeparatorHorizontal();
    m_layout->addWidget(m_nameLine);
    m_layout->addWidget(s1);
    m_layout->addWidget(m_passwdNew);
    m_layout->addWidget(s2);
    m_layout->addWidget(m_passwdRepeat);
    m_layout->addWidget(s3);
    m_layout->addWidget(m_autoLogin);
}

void CreateUserPanel::initConfirmLine()
{
    m_confirmLine = new AccountConfirmButtonLine();
    connect(m_confirmLine, &AccountConfirmButtonLine::cancel, this, &CreateUserPanel::onCancel);
    connect(m_confirmLine, &AccountConfirmButtonLine::confirm, this, &CreateUserPanel::onConfirm);

    DSeparatorHorizontal *s4 = new DSeparatorHorizontal();
    m_layout->addWidget(m_confirmLine);
    m_layout->addWidget(s4);
}

bool CreateUserPanel::validate()
{
    QDBusPendingReply<bool, QString, int> reply = m_account->IsUsernameValid(m_nameLine->text().toLower());
    bool valid = reply.argumentAt(0).isValid() ? reply.argumentAt(0).toBool() : false;
    QString warningMsg = reply.argumentAt(1).isValid() ? reply.argumentAt(1).toString() : "";
    if (!valid){
        m_nameLine->showWarning(dgettext("dde-daemon", warningMsg.toUtf8().data()));
        return false;
    }

    if (m_passwdNew->text().isEmpty()){
        m_passwdNew->showWarning(tr("Password can not be empty."));
        return false;
    }

    if (m_passwdRepeat->text().isEmpty()){
        m_passwdRepeat->showWarning(tr("Password can not be empty."));
        return false;
    }

    reply = m_account->IsPasswordValid(m_passwdNew->text().toLower());
    reply.waitForFinished();
    valid = reply.argumentAt(0).isValid() ? reply.argumentAt(0).toBool() : false;
    warningMsg = reply.argumentAt(1).isValid() ? reply.argumentAt(1).toString() : "";
    if (!valid) {
        m_passwdNew->showWarning(dgettext("dde-daemon", warningMsg.toUtf8().data()));
        return false;
    }

    if (m_passwdRepeat->text() != m_passwdNew->text()){
        m_passwdRepeat->showWarning(tr("The two passwords do not match."));
        return false;
    }

    return true;
}

void CreateUserPanel::resetData()
{
    m_oldName = "";
    m_nameLine->setText("");
    m_nameLine->hideWarning();
    m_passwdNew->setText("");
    m_passwdNew->hideWarning();
    m_passwdNew->passwordEdit()->setEchoMode(QLineEdit::Password);
    m_passwdRepeat->setText("");
    m_passwdRepeat->hideWarning();
    m_passwdRepeat->passwordEdit()->setEchoMode(QLineEdit::Password);
    m_autoLogin->setCheck(false);
    m_randIcon = m_account->RandUserIcon().value();
    m_avatar->setIcon(m_randIcon);
}

void CreateUserPanel::onCancel()
{
    emit createCancel();
    resetData();
}

void CreateUserPanel::onConfirm()
{
    if (validate()){
        this->window()->setProperty("autoHide", false);
        // Create administrator user by default.
        QDBusPendingReply<> reply = m_account->CreateUser(m_nameLine->text().toLower(), "", 1);
        reply.waitForFinished();
        if (!reply.error().isValid())
            emit createConfirm();
        //delay to buff windows active change
        QTimer::singleShot(1000, this, SLOT(onCanHideControlCenter()));
    }
}

void CreateUserPanel::onUserAdded(const QString &path)
{
    DBusAccountUser *user = new DBusAccountUser(path, this);
    if (user->isValid()){
        if (!m_randIcon.isEmpty())
            user->SetIconFile(m_randIcon);
        if (!m_passwdNew->text().isEmpty() && m_passwdNew->text() == m_passwdRepeat->text())
        {
            QDBusPendingReply<> reply = user->SetPassword(m_passwdNew->text());
            reply.waitForFinished();

            if (m_autoLogin->check())
                user->SetAutomaticLogin(m_autoLogin->check());
        }

        resetData();
    }
}

void CreateUserPanel::onNameFocusChanged(bool focus)
{
    if (focus){
        m_nameLine->hideWarning();
        m_passwdNew->hideWarning();
        m_passwdRepeat->hideWarning();
    }
    else if (this->isActiveWindow() && m_nameLine->text().isEmpty())
        m_nameLine->showWarning(tr("Username can not be empty."));
}

void CreateUserPanel::onPasswdFocusChanged(bool focus)
{
    if (focus){
        m_passwdNew->hideWarning();
        m_passwdRepeat->hideWarning();

        if (m_nameLine->text().isEmpty())
            m_nameLine->showWarning(tr("Username can not be empty."));
        else if (!m_passwdRepeat->text().isEmpty() && m_passwdRepeat->text() != m_passwdNew->text())
            m_passwdRepeat->showWarning(tr("The two passwords do not match."));
    }
    else if (this->isActiveWindow() && m_passwdNew->text().isEmpty() && !m_nameLine->text().isEmpty())
        m_passwdNew->showWarning(tr("Password can not be empty."));
}

void CreateUserPanel::onPasswdRepeatFocusChanged(bool focus)
{
    if (focus){
        m_passwdNew->hideWarning();
        m_passwdRepeat->hideWarning();

        if (m_nameLine->text().isEmpty())
            m_nameLine->showWarning(tr("Username can not be empty."));
    }
    else if (this->isActiveWindow() && !m_passwdNew->text().isEmpty() && m_passwdRepeat->text().isEmpty())
        m_passwdRepeat->showWarning(tr("The two passwords do not match."));
}

void CreateUserPanel::onNameChanged(const QString &name)
{
    m_nameLine->hideWarning();
    if (!name.isEmpty()){
        QDBusPendingReply<bool, QString, int> reply = m_account->IsUsernameValid(name.toLower());
        bool nameValid = reply.argumentAt(0).isValid() ? reply.argumentAt(0).toBool() : false;
        QString warningMsg = reply.argumentAt(1).isValid() ? reply.argumentAt(1).toString() : "";
        int validCode = reply.argumentAt(2).isValid() ? reply.argumentAt(2).toInt() : -1;
        if (nameValid){
            m_oldName = name;
            m_newNameLabel->setText(name.toLower());
        }
        else{
            if (validCode == 4 || validCode == 5)//4:NameExist,5:SystemUsed
                m_oldName = name;
            else{
                m_nameLine->setText(m_oldName.toLower());
            }
            m_newNameLabel->setText(m_oldName.isEmpty() ? tr("new user") : m_oldName.toLower());

            m_nameLine->showWarning(dgettext("dde-daemon", warningMsg.toUtf8().data()));
        }
    }
    else {
        m_newNameLabel->setText(tr("new user"));
        m_oldName = "";
    }
}

void CreateUserPanel::onPasswdChanged(const QString &)
{
    m_passwdNew->hideWarning();
    m_passwdRepeat->hideWarning();
    if (!m_passwdRepeat->text().isEmpty() && m_passwdNew->text() != m_passwdRepeat->text())
        m_passwdRepeat->showWarning(tr("The two passwords do not match."));
}

void CreateUserPanel::onPasswdRepeatChanged(const QString &passwd)
{
    m_passwdRepeat->hideWarning();
    if (!m_passwdRepeat->text().isEmpty() && m_passwdNew->text().indexOf(passwd, 0) != 0)
        m_passwdRepeat->showWarning(tr("The two passwords do not match."));
}

