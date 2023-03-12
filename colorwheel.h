// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QMainWindow>

class ColorwheelPrivate;
class Colorwheel : public QWidget
{
    Q_OBJECT
    public:
        Colorwheel(QWidget* parent = nullptr);
        virtual ~Colorwheel();
        qreal angle() const;
        qreal markerSize() const;
        qreal borderOpacity() const;
        qreal backgroundOpacity() const;
        QList<QColor> colors();
        int colorAt(const QPoint& point) const;
        bool isSaturationVisible() const;
        bool isIQLineVisible() const;
        int selected() const;
    
    public Q_SLOTS:
        void setAngle(qreal angle);
        void setBackgroundOpacity(qreal opacity);
        void setBorderOpacity(qreal opacity);
        void setColors(const QList<QColor>& colors);
        void setIQLineVisible(bool visible);
        void setMarkerSize(qreal size);
        void setSaturationVisible(bool visible);
        void setSelected(int selected);
    
    protected:
        void paintEvent(QPaintEvent *event) override;
        
    private:
        QScopedPointer<ColorwheelPrivate> p;
};
