// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QMainWindow>

class PickerPrivate;
class Picker : public QWidget
{
    Q_OBJECT
    public:
        Picker();
        virtual ~Picker();
        QColor color();
        void paintEvent(QPaintEvent* event) override;
    
    public Q_SLOTS:
        void setColor(QColor color);
    
    Q_SIGNALS:
        void triggered();
    
    private:
        QScopedPointer<PickerPrivate> p;
};
