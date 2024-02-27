// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QWidget>

class DraggerPrivate;
class Dragger: public QWidget
{
    Q_OBJECT
    public:
        Dragger(QWidget* parent = nullptr);
        virtual ~Dragger();
        void paintEvent(QPaintEvent* event) override;
        QRect dragRect() const;

    public Q_SLOTS:
        void update(const QPoint& position);
    
    Q_SIGNALS:
        void triggered();
        void closed();
    
    private:
        QScopedPointer<DraggerPrivate> p;
};
