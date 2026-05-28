// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QWidget>

class PickerPrivate;

/**
 * @class Picker
 * @brief Transparent overlay widget for previewing the currently sampled color.
 *
 * Tracks the global cursor position, draws a color preview/crosshair and emits
 * signals when the pick operation is triggered or closed.
 */
class Picker : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs a Picker widget.
     */
    Picker(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Picker widget.
     */
    virtual ~Picker();

    /**
     * @brief Returns the currently previewed color.
     */
    QColor color();

    /**
     * @brief Paints the picker overlay.
     */
    void paintEvent(QPaintEvent* event) override;

public Q_SLOTS:
    /**
     * @brief Sets the currently previewed color.
     */
    void setColor(const QColor& color);

    /**
     * @brief Updates the picker position in global screen coordinates.
     */
    void update(const QPoint& position);

Q_SIGNALS:
    /**
     * @brief Emitted when the current color pick is confirmed.
     */
    void triggered();

    /**
     * @brief Emitted when the picker is closed or cancelled.
     */
    void closed();

private:
    QScopedPointer<PickerPrivate> p;
};
