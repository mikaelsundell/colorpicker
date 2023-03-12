// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include "colorpicker.h"

#include <QWidget>

namespace mac
{
    void setupMac();
    void setupOverlay(WId wid);
    QPixmap grabDisplayPixmap(int x, int y, int width, int height);
    QPixmap grabDisplayPixmap(int x, int y, int width, int height, WId excludeWid);
}
