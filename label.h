// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <QLabel>

class LabelPrivate;

/**
 * @class Label
 * @brief Clickable label widget with optional read-only behavior.
 *
 * Extends QLabel with custom painting and a triggered() signal for user
 * interaction.
 */
class Label : public QLabel {
    Q_OBJECT

public:
    /**
     * @brief Constructs a Label widget.
     */
    Label(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Label widget.
     */
    virtual ~Label();

    /**
     * @brief Paints the label.
     */
    void paintEvent(QPaintEvent* event) override;

public Q_SLOTS:
    /**
     * @brief Sets whether the label is read-only.
     */
    void setReadOnly(bool readOnly);

Q_SIGNALS:
    /**
     * @brief Emitted when the label is triggered by user interaction.
     */
    void triggered();

private:
    QScopedPointer<LabelPrivate> p;
};
