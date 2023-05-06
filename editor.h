// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QDialog>

class EditorPrivate;
class Editor : public QDialog
{
    Q_OBJECT
    public:
        Editor(QWidget* parent = nullptr);
        virtual ~Editor();
        int maximum() const;
        int minimum() const;
        void setMaximum(int maximum);
        void setMinimum(int minimum);
        int value() const;
    
    public Q_SLOTS:
        void setValue(int value);
    
    Q_SIGNALS:
        void valueChanged(int);
    
    protected:
        void paintEvent(QPaintEvent *event) override;
    
    private:
        QScopedPointer<EditorPrivate> p;
};
