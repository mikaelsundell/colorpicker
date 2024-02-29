// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include <lcms2.h>

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QScopedPointer>

class ICCTransformPrivate;
class ICCTransform : public QObject
{
    Q_OBJECT
    public:
        static ICCTransform* instance();
        QString inputProfile() const;
        QString outputProfile() const;
        QRgb transformTo(QRgb image);
        QImage transformTo(const QImage& image);
        QRgb transformTo(QRgb color, const QString& profile, const QString& outputProfile);
        QImage transformTo(const QImage& image, const QString& profile, const QString& outputProfile);
   
    public Q_SLOTS:
        void setInputProfile(const QString& inputProfile);
        void setOutputProfile(const QString& displayProfile);
    
    private:
        ICCTransform();
        ~ICCTransform();
        ICCTransform(const ICCTransform&) = delete;
        ICCTransform& operator=(const ICCTransform&) = delete;
        class Deleter {
        public:
            static void cleanup(ICCTransform* pointer) {
                delete pointer;
            }
        };
        static QScopedPointer<ICCTransform, Deleter> pi;
        QScopedPointer<ICCTransformPrivate> p;
};
