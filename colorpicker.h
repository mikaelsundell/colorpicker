// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QMainWindow>

class ColorpickerPrivate;

/**
 * @class Colorpicker
 * @brief Main application window for the colorpicker tool.
 *
 * Handles color picking, cursor tracking and drag-and-drop input.
 */
class Colorpicker : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructs the main colorpicker window.
     */
    Colorpicker();

    /**
     * @brief Destroys the main colorpicker window.
     */
    virtual ~Colorpicker();

    /**
     * @brief Returns true if color picking is currently active.
     */
    bool active() const;

    /**
     * @struct PickEvent
     * @brief Describes a completed color pick event.
     */
    typedef struct {
        int displayNumber;   ///< Display index where the color was picked.
        QString iccProfile;  ///< ICC profile associated with the display.
        QPoint cursor;       ///< Global cursor position at the pick location.
    } PickEvent;

    /**
     * @struct MoveEvent
     * @brief Describes a cursor move event during color picking.
     */
    typedef struct {
        QPoint cursor;  ///< Global cursor position.
    } MoveEvent;

protected:
    /**
     * @brief Handles drag-enter events for supported dropped data.
     */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /**
     * @brief Handles drop events for supported dropped data.
     */
    void dropEvent(QDropEvent* event) override;

private:
    /**
     * @brief Registers platform and application event callbacks.
     */
    void registerEvents();

    /**
     * @brief Handles a completed color pick event.
     */
    void pickEvent(PickEvent event);

    /**
     * @brief Handles a cursor move event during color picking.
     */
    void moveEvent(MoveEvent event);

    QScopedPointer<ColorpickerPrivate> p;
};
