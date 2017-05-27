/* * This file is part of Maliit framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 * Copyright 2013-2017 Myriad Group AG. All Rights Reserved.
 */

#ifndef MINPUTCONTEXT_H
#define MINPUTCONTEXT_H

#include "dbusserverconnection.h"

#include <QMetaType>
#include <QObject>
#include <QRect>

class MInputContext : public QObject
{
    Q_OBJECT

public:
    enum OrientationAngle {
        Angle0   =   0,
        Angle90  =  90,
        Angle180 = 180,
        Angle270 = 270
    };

    MInputContext();
    virtual ~MInputContext();

    Q_INVOKABLE void reset();
    Q_INVOKABLE void showInputPanel();
    Q_INVOKABLE void hideInputPanel();
    Q_INVOKABLE void updateServerOrientation(MInputContext::OrientationAngle angle);
    Q_INVOKABLE void updateStateInfo(QMap<QString, QVariant> stateInfo, bool focusChanged);

    virtual void onHideInputMethod() = 0;
    virtual void onCommitString(const QString &string,
                       int replacementStart, int replacementLength, int cursorPos) = 0;
    virtual void onUpdatePreedit(const QString &string,
                       int replacementStart, int replacementLength, int cursorPos) = 0;
    virtual void onKeyEvent(int key, bool down)= 0;
    virtual void onUpdateInputMethodArea(int x, int y, int w, int h) = 0;
    virtual void onConnectionReady() = 0;
    virtual QMap<QString, QVariant> getStateInformation() = 0;

public Q_SLOTS:
    // Hooked up to the input method server
    void activationLostEvent();
    void imInitiatedHide();

    void commitString(const QString &string, int replacementStart = 0,
                      int replacementLength = 0, int cursorPos = -1);

    void updatePreedit(const QString &string, const QList<Maliit::PreeditTextFormat> &preeditFormats,
                       int replacementStart = 0, int replacementLength = 0, int cursorPos = -1);

    void keyEvent(int type, int key, int modifiers, const QString &text, bool autoRepeat,
                  int count, Maliit::EventRequestType requestType = Maliit::EventRequestBoth);

    void updateInputMethodArea(const QRect &rect);
    void setGlobalCorrectionEnabled(bool);
    void onInvokeAction(const QString &action, const QKeySequence &sequence);
    void setRedirectKeys(bool enabled);
    void setDetectableAutoRepeat(bool enabled);
    void setSelection(int start, int length);
    void getSelection(QString &selection, bool &valid) const;
    void setLanguage(const QString &language);
    // End input method server connection slots.

private Q_SLOTS:
    void onDBusDisconnection();
    void onDBusConnection();

private:
    Q_DISABLE_COPY(MInputContext)

    enum InputPanelState {
        InputPanelShowPending,   // input panel showing requested, but activation pending
        InputPanelShown,
        InputPanelHidden
    };

    void connectInputMethodServer();

    static bool debug;

    InputPanelState inputPanelState;
    DBusServerConnection *imServer;
    bool active; // is connection active
    bool mIMServerRestart; // Maliit server crashes/restart
    QString preedit;
    MInputContext::OrientationAngle mAngle;
};

Q_DECLARE_METATYPE(MInputContext::OrientationAngle)

#endif
