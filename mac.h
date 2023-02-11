// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include "colorpicker.h"

namespace mac
{
    void setupMac();
    QPixmap grabDisplayPixmap(int x, int y, int width, int height);
}
