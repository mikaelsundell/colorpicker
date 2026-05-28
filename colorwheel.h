// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QMainWindow>

class ColorwheelPrivate;

/**
 * @class Colorwheel
 * @brief Widget for displaying and interacting with a color wheel.
 *
 * Provides color marker visualization, selection mapping, saturation guides,
 * labels and segmented/smooth wheel rendering.
 */
class Colorwheel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a Colorwheel widget.
     */
    Colorwheel(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Colorwheel widget.
     */
    virtual ~Colorwheel();

    /**
     * @brief Returns the current wheel rotation angle.
     */
    qreal angle() const;

    /**
     * @brief Returns the marker size factor.
     */
    qreal markerSize() const;

    /**
     * @brief Returns the border opacity.
     */
    qreal borderOpacity() const;

    /**
     * @brief Returns the wheel background opacity.
     */
    qreal backgroundOpacity() const;

    /**
     * @brief Returns the list of displayed colors.
     */
    QList<QPair<QColor, QPair<QString, QString>>> colors();

    /**
     * @brief Maps a widget-space point to the selected marker index.
     */
    int mapToSelected(const QPoint& point) const;

    /**
     * @brief Maps a widget-space point to a color on the wheel.
     */
    QColor mapToColor(const QPoint& point) const;

    /**
     * @brief Maps a widget-space point to a color while preserving value.
     */
    QColor mapToColor(const QColor& color, const QPoint& point) const;

    /**
     * @brief Returns true if the I/Q reference line is visible.
     */
    bool isIQLineVisible() const;

    /**
     * @brief Returns true if saturation guide circles are visible.
     */
    bool isSaturationVisible() const;

    /**
     * @brief Returns true if color labels are visible.
     */
    bool isLabelsVisible() const;

    /**
     * @brief Returns true if the segmented wheel is enabled.
     */
    bool isSegmented() const;

    /**
     * @brief Returns the selected color marker index.
     */
    qsizetype selected() const;

    /**
     * @brief Returns true if a marker is currently selected.
     */
    bool hasSelection() const;

    /**
     * @brief Returns the saturation zoom factor.
     */
    qreal zoomFactor() const;

public Q_SLOTS:
    /**
     * @brief Sets the wheel rotation angle.
     */
    void setAngle(qreal angle);

    /**
     * @brief Sets the wheel background opacity.
     */
    void setBackgroundOpacity(qreal opacity);

    /**
     * @brief Sets the border opacity.
     */
    void setBorderOpacity(qreal opacity);

    /**
     * @brief Sets the displayed colors.
     */
    void setColors(const QList<QPair<QColor, QPair<QString, QString>>> colors, bool selected = true);

    /**
     * @brief Sets whether the I/Q reference line is visible.
     */
    void setIQLineVisible(bool visible);

    /**
     * @brief Sets whether color labels are visible.
     */
    void setLabelsVisible(bool visible);

    /**
     * @brief Sets the marker size factor.
     */
    void setMarkerSize(qreal size);

    /**
     * @brief Sets whether saturation guide circles are visible.
     */
    void setSaturationVisible(bool visible);

    /**
     * @brief Sets whether the segmented wheel is enabled.
     */
    void setSegmented(bool segmented);

    /**
     * @brief Sets the selected color marker index.
     */
    void setSelected(qsizetype selected);

    /**
     * @brief Sets the saturation zoom factor.
     */
    void setZoomFactor(qreal factor);

protected:
    /**
     * @brief Handles widget resize events.
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Handles widget show events.
     */
    void showEvent(QShowEvent* event) override;

    /**
     * @brief Paints the color wheel.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    QScopedPointer<ColorwheelPrivate> p;
};
