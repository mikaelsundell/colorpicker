// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QWidget>

class PickerPrivate;
class Picker : public QWidget
{
    Q_OBJECT
    public:
        Picker(QWidget* parent = nullptr);
        virtual ~Picker();
        QColor color();
        void paintEvent(QPaintEvent* event) override;
    
    public Q_SLOTS:
        void setColor(const QColor& color);
        void update(const QPoint& position);
    
    Q_SIGNALS:
        void triggered();
        void closed();
    
    private:
        QScopedPointer<PickerPrivate> p;
};
