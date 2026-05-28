// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QWidget>

class DraggerPrivate;

/**
 * @class Dragger
 * @brief Transparent overlay widget for dragging out a screen-space rectangle.
 *
 * Tracks cursor movement, draws a crosshair/selection sweep and emits signals
 * when the drag operation is completed or cancelled.
 */
class Dragger : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a Dragger widget.
     */
    Dragger(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Dragger widget.
     */
    virtual ~Dragger();

    /**
     * @brief Paints the drag overlay.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Returns the current drag rectangle in global screen coordinates.
     */
    QRect dragRect() const;

public Q_SLOTS:
    /**
     * @brief Updates the dragger position in global screen coordinates.
     */
    void update(const QPoint& position);

Q_SIGNALS:
    /**
     * @brief Emitted when the drag operation is completed.
     */
    void triggered();

    /**
     * @brief Emitted when the dragger is closed or cancelled.
     */
    void closed();

private:
    QScopedPointer<DraggerPrivate> p;
};
