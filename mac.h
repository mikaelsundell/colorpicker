// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QString>
#include <QWidget>

namespace mac
{
    struct IccProfile {
        int screenNumber;
        QString displayProfileUrl;
    };
    void setDarkAppearance();
    void setTopLevel(WId wid);
    void hideCursor();
    void showCursor();
    QImage grabImage(int x, int y, int width, int height, WId windowId);
    IccProfile grabIccProfile(int x, int y);
    IccProfile grabIccProfile(WId wid);
    QString grabIccProfileUrl(WId wid);
    QPoint fromNativeCursor(float x, float y);
    QPointF toNativeCursor(int x, int y);
    void console(QString message);
}
