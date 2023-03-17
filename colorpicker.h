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
        bool active() const;
        typedef struct {
            QString displayProfile;
            int displayNumber;
            QPoint cursor;
        } DisplayEvent;
        typedef struct {
            QPoint cursor;
        } MouseEvent;
    private:
        void registerEvents();
        void displayEvent(DisplayEvent event);
        void mouseEvent(MouseEvent event);
        QScopedPointer<ColorpickerPrivate> p;
};
