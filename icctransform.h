// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <lcms2.h>

#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QScopedPointer>

class ICCTransformPrivate;

/**
 * @class ICCTransform
 * @brief Singleton helper for ICC-based color transforms.
 *
 * Provides color and image mapping between input, display and explicit ICC
 * profiles using Little CMS.
 */
class ICCTransform : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Returns the shared ICCTransform instance.
     */
    static ICCTransform* instance();

    /**
     * @brief Returns the current input ICC profile path.
     */
    QString inputProfile() const;

    /**
     * @brief Returns the current output ICC profile path.
     */
    QString outputProfile() const;

    /**
     * @brief Maps a color using the current input and output profiles.
     */
    QRgb map(QRgb color);

    /**
     * @brief Maps an image using the current input and output profiles.
     */
    QImage map(const QImage& image);

    /**
     * @brief Maps a color between explicit ICC profile paths.
     */
    QRgb map(QRgb color, const QString& profile, const QString& outputProfile);

    /**
     * @brief Maps an image between explicit ICC profile paths.
     */
    QImage map(const QImage& image, const QString& profile, const QString& outputProfile);

    /**
     * @brief Maps a color from a QColorSpace to an explicit output ICC profile.
     */
    QRgb map(QRgb color, const QColorSpace& colorSpace, const QString& outputProfile);

    /**
     * @brief Maps an image from a QColorSpace to an explicit output ICC profile.
     */
    QImage map(const QImage& image, const QColorSpace& colorSpace, const QString& outputProfile);

public Q_SLOTS:
    /**
     * @brief Sets the current input ICC profile path.
     */
    void setInputProfile(const QString& inputProfile);

    /**
     * @brief Sets the current output ICC profile path.
     */
    void setOutputProfile(const QString& outputProfile);

Q_SIGNALS:
    /**
     * @brief Emitted when the input ICC profile changes.
     */
    void inputProfileChanged(const QString& inputProfile);

    /**
     * @brief Emitted when the output ICC profile changes.
     */
    void outputProfileChanged(const QString& outputProfile);

private:
    ICCTransform();
    ~ICCTransform();

    ICCTransform(const ICCTransform&) = delete;
    ICCTransform& operator=(const ICCTransform&) = delete;

    class Deleter {
    public:
        static void cleanup(ICCTransform* pointer) { delete pointer; }
    };

    static QScopedPointer<ICCTransform, Deleter> pi;

    QScopedPointer<ICCTransformPrivate> p;
};
