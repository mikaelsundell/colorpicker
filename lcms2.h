// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <lcms2.h>
#include <QImage>

namespace lcms2
{
    QImage convertImage(QImage& image, QVector<cmsHPROFILE> profiles, int intent, cmsUInt32Number flags);
    QColor convertColor(QColor color, QString profile, QString displayProfile);
}
