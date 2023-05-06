// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QLabel>

class LabelPrivate;
class Label : public QLabel
{
    Q_OBJECT
    public:
        Label(QWidget* parent = nullptr);
        virtual ~Label();
        void paintEvent(QPaintEvent* event) override;

    public Q_SLOTS:
        void setReadOnly(bool readOnly);
    
    Q_SIGNALS:
        void triggered();
    
    private:
        QScopedPointer<LabelPrivate> p;
};
