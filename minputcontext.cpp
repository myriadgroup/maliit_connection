/* * This file is part of Maliit framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 * Copyright (C) 2013 Jolla Ltd.
 *
 * All rights reserved.
 *
 * Contact: maliit-discuss@lists.maliit.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

/*
 * Copyright 2013-2014 Myriad Group AG. All Rights Reserved.
 */

#include "minputcontext.h"
#include <QDebug>

bool MInputContext::debug = false;

MInputContext::MInputContext()
    : imServer(NULL),
      active(false),
      inputPanelState(InputPanelHidden),
      mIMServerRestart(false),
      mAngle(Angle0),
      mSurroundingText(""),
      mCursorPos(0)
{
    if (debug) qDebug() << "MInputContext()";

    qRegisterMetaType<MInputContext::OrientationAngle >();

    QSharedPointer<Maliit::InputContext::DBus::Address> address(new Maliit::InputContext::DBus::DynamicAddress);
    imServer = new DBusServerConnection(address);
    connectInputMethodServer();
}

MInputContext::~MInputContext()
{
    delete imServer;
}

void MInputContext::connectInputMethodServer()
{
    if (debug) qDebug() << "connectInputMethodServer()";

    connect(imServer, SIGNAL(connected()), this, SLOT(onDBusConnection()));
    connect(imServer, SIGNAL(disconnected()), this, SLOT(onDBusDisconnection()));

    // Hook up incoming communication from input method server
    connect(imServer, SIGNAL(activationLostEvent()), this, SLOT(activationLostEvent()));

    connect(imServer, SIGNAL(imInitiatedHide()), this, SLOT(imInitiatedHide()));

    connect(imServer, SIGNAL(commitString(QString,int,int,int)),
            this, SLOT(commitString(QString,int,int,int)));

    connect(imServer, SIGNAL(updatePreedit(QString,QList<Maliit::PreeditTextFormat>,int,int,int)),
            this, SLOT(updatePreedit(QString,QList<Maliit::PreeditTextFormat>,int,int,int)));

    connect(imServer, SIGNAL(keyEvent(int,int,int,QString,bool,int,Maliit::EventRequestType)),
            this, SLOT(keyEvent(int,int,int,QString,bool,int,Maliit::EventRequestType)));

    connect(imServer, SIGNAL(updateInputMethodArea(QRect)),
            this, SLOT(updateInputMethodArea(QRect)));

    connect(imServer, SIGNAL(setGlobalCorrectionEnabled(bool)),
            this, SLOT(setGlobalCorrectionEnabled(bool)));

    connect(imServer, SIGNAL(invokeAction(QString,QKeySequence)), this, SLOT(onInvokeAction(QString,QKeySequence)));

    connect(imServer, SIGNAL(setRedirectKeys(bool)), this, SLOT(setRedirectKeys(bool)));

    connect(imServer, SIGNAL(setDetectableAutoRepeat(bool)),
            this, SLOT(setDetectableAutoRepeat(bool)));

    connect(imServer, SIGNAL(setSelection(int,int)),
            this, SLOT(setSelection(int,int)));

    connect(imServer, SIGNAL(getSelection(QString&,bool&)),
            this, SLOT(getSelection(QString&, bool&)));

    connect(imServer, SIGNAL(setLanguage(QString)),
            this, SLOT(setLanguage(QString)));
}

void MInputContext::setLanguage(const QString &language)
{
    if (debug) qDebug() << "unimplemented setLanuage()";
}

void MInputContext::reset()
{
    if (debug) qDebug() << "reset()";

    const bool hadPreedit = !preedit.isEmpty();

    // reset input method server, preedit requires synchronization.
    // rationale: input method might be autocommitting existing preedit without
    // user interaction.
    imServer->reset(hadPreedit);
}

void MInputContext::updateEditorInfo(bool isFocusChanged, int type, bool passwd, QString text, int cursor, bool selected, bool force)
{
    // Clear preedit String on im server side to avoid showing up
    // on new edit box
    if (isFocusChanged) {
        reset();
    }

    bool changed = (force || isFocusChanged || (type != contentType) || (hiddenText != passwd)
                   || (text != mSurroundingText) || (cursor != mCursorPos) || (mTextSelected != selected));
    if (!changed) {
        if (debug) qDebug() << "Input type is NOT changed, return!";
        return;
    }

    contentType = type;
    hiddenText = passwd;
    mSurroundingText = text;
    mCursorPos = cursor;
    mTextSelected = selected;

    QMap<QString, QVariant> stateInfo;
    stateInfo["focusState"] = true;
    stateInfo["contentType"] = contentType;
    stateInfo["hiddenText"] = hiddenText;
    stateInfo["surroundingText"] = mSurroundingText;
    stateInfo["cursorPosition"] = mCursorPos;
    stateInfo["hasSelection"] = mTextSelected;

    imServer->updateWidgetInformation(stateInfo, isFocusChanged);
}

void MInputContext::onInvokeAction(const QString &action, const QKeySequence &sequence)
{
    if (debug) qDebug() << "unimplemented onInvokeAction()";
}

void MInputContext::updateServerOrientation(MInputContext::OrientationAngle angle)
{
    if (debug) qDebug() << "updateServerOrientation(): angle = " << angle;

    if (active) {
        imServer->appOrientationChanged(static_cast<int>(angle));
    }
    mAngle = angle;
}

void MInputContext::showInputPanel()
{
    if (debug) qDebug() << "showInputPanel() active = " << active;

    if (!active) {
        imServer->activateContext();
        active = true;
        imServer->appOrientationChanged(mAngle);
    }

    imServer->showInputMethod();
    inputPanelState = InputPanelShown;
}

void MInputContext::hideInputPanel()
{
    if (debug) qDebug() << "hideInputPanel()";

    imServer->hideInputMethod();
    inputPanelState = InputPanelHidden;
}

void MInputContext::activationLostEvent()
{
    // This method is called when activation was gracefully lost.
    // There is similar cleaning up done in onDBusDisconnection.
    if (debug) qDebug() << "activationLostEvent()";
    active = false;
    inputPanelState = InputPanelHidden;
}

void MInputContext::imInitiatedHide()
{
    if (debug) qDebug() << "imInitiatedHide()";

    onHideInputMethod();
}

void MInputContext::commitString(const QString &string, int replacementStart,
                                 int replacementLength, int cursorPos)
{
    if (debug) qDebug() << "commitString()";

    if (imServer->pendingResets()) {
        return;
    }

    preedit.clear();

    onCommitString(string);
}

void MInputContext::updatePreedit(const QString &string, const QList<Maliit::PreeditTextFormat> &preeditFormats,
                       int replacementStart, int replacementLength, int cursorPos)
{
    if (debug) {
        qDebug() << "updatePreedit()" << "preedit:" << string
                 << ", replacementStart:" << replacementStart
                 << ", replacementLength:" << replacementLength
                 << ", cursorPos:" << cursorPos;
    }

    if (imServer->pendingResets()) {
        return;
    }

    preedit = string;

    onUpdatePreedit(string, replacementStart, replacementLength);
}

void MInputContext::keyEvent(int type, int key, int modifiers, const QString &text,
                             bool autoRepeat, int count,
                             Maliit::EventRequestType requestType)
{
    if (debug) qDebug() << "keyEvent()";

    bool down = (type == QEvent::KeyPress);
    onKeyEvent(key, down);
}

void MInputContext::updateInputMethodArea(const QRect &rect)
{
    int x = rect.x();
    int y = rect.y();
    int w = rect.width();
    int h = rect.height();

    onUpdateInputMethodArea(x, y, w, h);
}

void MInputContext::setGlobalCorrectionEnabled(bool enabled)
{
    if (debug) qDebug() << "setGlobalCorrectionEnabled(), enabled = " << enabled;
    updateEditorInfo(true, contentType, hiddenText, mSurroundingText.toUtf8().data(), mCursorPos, true);
}

void MInputContext::onDBusDisconnection()
{
    if (debug) qDebug() << "onDBusDisconnection()";
    active = false;
    mIMServerRestart = true;

    updateInputMethodArea(QRect());
}

void MInputContext::onDBusConnection()
{
    if (debug) qDebug() << "onDBusConnection()";
    active = false;
    onConnectionReady();
    if (mIMServerRestart && inputPanelState == InputPanelShown) {
        updateEditorInfo(true, contentType, hiddenText, mSurroundingText.toUtf8().data(), mCursorPos, true);
        showInputPanel();
        mIMServerRestart = false;
    }
}

void MInputContext::setRedirectKeys(bool enabled)
{
    if (debug) qDebug() << "unimplemented setRedirectKeys()";
}

void MInputContext::setDetectableAutoRepeat(bool enabled)
{
    if (debug) qDebug() << "unimplemented setDetectableAutoRepeat()";
}

void MInputContext::setSelection(int start, int length)
{
    if (debug) qDebug() << "unimplemented setSelection()";
}

void MInputContext::getSelection(QString &selection, bool &valid) const
{
    if (debug) if (debug) qDebug() << "unimplemented getSelection()";
}
