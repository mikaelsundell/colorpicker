// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QDialog>

class EditorPrivate;

/**
 * @class Editor
 * @brief Popup editor dialog for adjusting an integer value.
 *
 * Provides a lightweight frameless dialog with a slider-based value control.
 */
class Editor : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructs an Editor dialog.
     */
    Editor(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Editor dialog.
     */
    virtual ~Editor();

    /**
     * @brief Returns the maximum allowed value.
     */
    int maximum() const;

    /**
     * @brief Returns the minimum allowed value.
     */
    int minimum() const;

    /**
     * @brief Returns the current value.
     */
    int value() const;

    /**
     * @brief Sets the maximum allowed value.
     */
    void setMaximum(int maximum);

    /**
     * @brief Sets the minimum allowed value.
     */
    void setMinimum(int minimum);

public Q_SLOTS:
    /**
     * @brief Sets the current value.
     */
    void setValue(int value);

Q_SIGNALS:
    /**
     * @brief Emitted when the current value changes.
     */
    void valueChanged(int value);

protected:
    /**
     * @brief Paints the editor background.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    QScopedPointer<EditorPrivate> p;
};
