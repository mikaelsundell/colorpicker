// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QWidget>

class GridPrivate;
class Grid : public QWidget
{
    Q_OBJECT
    public:
        Grid();
        virtual ~Grid();
        QColor borderColor();
        void paintEvent(QPaintEvent* event) override;

    public Q_SLOTS:
        void setBorderColor(const QColor& color);
        void update(const QPoint& position);
    
    Q_SIGNALS:
        void triggered();
        void closed();
    
    private:
        QScopedPointer<GridPrivate> p;
};
