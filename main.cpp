// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include <QApplication>
#include "colorpicker.h"

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Colorpicker* colorpicker = new Colorpicker();
    colorpicker->show();
    return app.exec();
}
