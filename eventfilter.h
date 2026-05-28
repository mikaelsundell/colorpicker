// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QMainWindow>

/**
 * @class Eventfilter
 * @brief Event filter that emits a signal when a press event is detected.
 *
 * Provides a small QObject-based helper for intercepting events from another
 * object and forwarding press interaction through the pressed() signal.
 */
class Eventfilter : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs an Eventfilter.
     */
    Eventfilter(QObject* object = nullptr);

    /**
     * @brief Destroys the Eventfilter.
     */
    virtual ~Eventfilter();

Q_SIGNALS:
    /**
     * @brief Emitted when a press event is detected.
     */
    void pressed();

protected:
    /**
     * @brief Filters events from the watched object.
     */
    bool eventFilter(QObject* obj, QEvent* event) override;
};
