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
        QList<QPair<QColor, QString>> colors();
        int mapToSelected(const QPoint& point) const;
        QColor mapToColor(const QPoint& point) const;
        QColor mapToColor(const QColor& color, const QPoint& point) const;
        bool isIQLineVisible() const;
        bool isSaturationVisible() const;
        bool isLabelsVisible() const;
        qsizetype selected() const;
        bool hasSelection() const;
        qreal zoomFactor() const;
    
    public Q_SLOTS:
        void setAngle(qreal angle);
        void setBackgroundOpacity(qreal opacity);
        void setBorderOpacity(qreal opacity);
        void setColors(const QList<QPair<QColor, QString>> colors, bool selected = true);
        void setIQLineVisible(bool visible);
        void setLabelsVisible(bool visible);
        void setMarkerSize(qreal size);
        void setSaturationVisible(bool visible);
        void setSelected(qsizetype selected);
        void setZoomFactor(qreal factor);

    protected:
        void paintEvent(QPaintEvent *event) override;
        
    private:
        QScopedPointer<ColorwheelPrivate> p;
};
