// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QString>
#include <QWidget>

namespace mac {

/**
 * @struct IccProfile
 * @brief Describes the ICC profile associated with a macOS display.
 */
struct IccProfile {
    int screenNumber;           ///< Display index associated with the profile.
    QString displayProfileUrl;  ///< ICC profile URL/path for the display.
};

/**
 * @brief Enables dark appearance for the application.
 */
void
setDarkAppearance();

/**
 * @brief Sets the native window level for a Qt widget window.
 */
void
setTopLevel(WId wid);

/**
 * @brief Hides the native macOS cursor.
 */
void
hideCursor();

/**
 * @brief Shows the native macOS cursor.
 */
void
showCursor();

/**
 * @brief Captures an image from a native window or screen region.
 */
QImage
grabImage(int x, int y, int width, int height, WId windowId);

/**
 * @brief Returns the ICC profile for the display at a screen position.
 */
IccProfile
grabIccProfile(int x, int y);

/**
 * @brief Returns the ICC profile for the display containing a native window.
 */
IccProfile
grabIccProfile(WId wid);

/**
 * @brief Returns the ICC profile URL/path for the display containing a native window.
 */
QString
grabIccProfileUrl(WId wid);

/**
 * @brief Converts native macOS cursor coordinates to Qt screen coordinates.
 */
QPoint
fromNativeCursor(float x, float y);

/**
 * @brief Converts Qt screen coordinates to native macOS cursor coordinates.
 */
QPointF
toNativeCursor(int x, int y);

/**
 * @brief Writes a message to the native macOS console/log.
 */
void
console(QString message);

}  // namespace mac
