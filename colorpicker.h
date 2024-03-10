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
            int displayNumber;
            QString iccProfile;
            QPoint cursor;
        } PickEvent;
        typedef struct {
            QPoint cursor;
        } MoveEvent;
    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
    private:
        void registerEvents();
        void pickEvent(PickEvent event);
        void moveEvent(MoveEvent event);
        QScopedPointer<ColorpickerPrivate> p;
};
