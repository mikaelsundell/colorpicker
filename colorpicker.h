// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QMainWindow>

class ColorpickerPrivate;
class Colorpicker : public QMainWindow
{
    Q_OBJECT
    public:
        Colorpicker();
        virtual ~Colorpicker();
        void update();
        typedef struct {
            QString displayProfile;
            int displayNumber;
            QPoint cursor;
        } DisplayEvent;
    private:
        void registerEvents();
        void updateEvents(DisplayEvent event);
        QScopedPointer<ColorpickerPrivate> p;
};
