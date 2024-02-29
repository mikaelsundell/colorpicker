// Copyright 2022-present Contributors to the vectorscope project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorman

#include "icctransform.h"
#include <QApplication>
#include <QDir>
#include <QMap>
#include <QPointer>
#include <QMutex>

QScopedPointer<ICCTransform, ICCTransform::Deleter> ICCTransform::pi;

class ICCTransformPrivate : public QObject
{
    Q_OBJECT
    public:
        ICCTransformPrivate();
        ~ICCTransformPrivate();
        cmsUInt32Number convertFormat(QImage::Format format);
        cmsHTRANSFORM convertTransform(const QString& profile, cmsUInt32Number format, const QString& outProfile);
        QRgb transformTo(QRgb color, QString profile, QString outProfile);
        QImage transformTo(QImage image, QString profile, QString outProfile);
    
    public:
        QString inputProfile;
        QString outputProfile;
        QMap<QString, QMap<cmsUInt32Number, QMap<QString, cmsHTRANSFORM> > > cache;
        QPointer<ICCTransform> transform;
};

ICCTransformPrivate::ICCTransformPrivate()
{
}

ICCTransformPrivate::~ICCTransformPrivate()
{
    for(QString profile : cache.keys()) {
        for (cmsUInt32Number format : cache[profile].keys()) {
            for (QString outputProfile : cache[profile][format].keys()) {
                cmsDeleteTransform(cache[profile][format][outputProfile]);
            }
        }
    }
    cache.clear();
}

cmsUInt32Number
ICCTransformPrivate::convertFormat(QImage::Format format)
{
    switch (format) {
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
        case QImage::Format_RGB32:
            return TYPE_BGRA_8;

        case QImage::Format_RGB888:
            return TYPE_RGB_8;

        case QImage::Format_RGBX8888:
        case QImage::Format_RGBA8888:
            return TYPE_RGBA_8;

        case QImage::Format_Grayscale8:
            return TYPE_GRAY_8;

        case QImage::Format_Grayscale16:
            return TYPE_GRAY_16;

        case QImage::Format_RGBA64:
        case QImage::Format_RGBX64:
            return TYPE_RGBA_16;
                
        case QImage::Format_BGR888:
            return TYPE_BGR_8;

        default:
            return 0;
    }
}

cmsHTRANSFORM
ICCTransformPrivate::convertTransform(const QString& profile, cmsUInt32Number format, const QString& outProfile)
{
    cmsHTRANSFORM transform = nullptr;
    if (!cache.contains(profile)) {
        cache.insert(profile, QMap<cmsUInt32Number, QMap<QString, cmsHTRANSFORM> >());
    }
    if (!cache[profile].contains(format)) {
        cache[profile].insert(format, QMap<QString, cmsHTRANSFORM>());
    }
    if (!cache[profile][format].contains(outProfile)) {
        cmsHPROFILE cmsProfile = cmsOpenProfileFromFile(profile.toLocal8Bit().constData(), "r");
        cmsHPROFILE cmsDisplayProfile = cmsOpenProfileFromFile(outProfile.toLocal8Bit().constData(), "r");
        cache[profile][format].insert(
            outProfile,
            cmsCreateTransform(cmsProfile, format, cmsDisplayProfile, format, INTENT_PERCEPTUAL, 0)
        );
        cmsCloseProfile(cmsProfile);
        cmsCloseProfile(cmsDisplayProfile);
    }
    return cache[profile][format][outProfile];
}

QRgb
ICCTransformPrivate::transformTo(QRgb color, QString profile, QString outProfile)
{
    cmsHTRANSFORM transform = convertTransform(
        profile, convertFormat(QImage::Format_RGB32), outProfile
    );
    QRgb transformColor;
    cmsDoTransform(transform, &color, &transformColor, 1);
    return transformColor;
}

QImage
ICCTransformPrivate::transformTo(QImage image, QString profile, QString outProfile)
{
    cmsHTRANSFORM transform = convertTransform(profile, convertFormat(image.format()), outProfile);
    QImage transformimage(image.width(), image.height(), image.format());
    cmsDoTransformLineStride(
        transform,
        image.constBits(),
        transformimage.bits(),
        image.width(),
        image.height(),
        static_cast<cmsUInt32Number>(image.bytesPerLine()),
        static_cast<cmsUInt32Number>(transformimage.bytesPerLine()),
        0,
        0);
    transformimage.setDevicePixelRatio(image.devicePixelRatio());
    return transformimage;
}

#include "icctransform.moc"

ICCTransform::ICCTransform()
: p(new ICCTransformPrivate())
{
}

ICCTransform::~ICCTransform()
{
    p->cache.clear();
}

ICCTransform*
ICCTransform::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!pi) {
        pi.reset(new ICCTransform());
    }
    return pi.data();
}

QString
ICCTransform::ICCTransform::inputProfile() const
{
    return p->inputProfile;
}

void
ICCTransform::ICCTransform::setInputProfile(const QString& inputProfile)
{
    p->inputProfile = inputProfile;
}

QString
ICCTransform::ICCTransform::outputProfile() const
{
    return p->outputProfile;
}

void
ICCTransform::ICCTransform::setOutputProfile(const QString& outputProfile)
{
    p->outputProfile = outputProfile; 
}

QRgb
ICCTransform::ICCTransform::transformTo(QRgb color)
{
    return p->transformTo(color, p->inputProfile, p->outputProfile);
}

QImage
ICCTransform::ICCTransform::transformTo(QImage image)
{
    return p->transformTo(image, p->inputProfile, p->outputProfile);
}

QRgb
ICCTransform::ICCTransform::transformTo(QRgb color, QString inputProfile, QString outputProfile)
{
    return p->transformTo(color, inputProfile, outputProfile);
}

QImage
ICCTransform::transformTo(QImage image, QString inputProfile, QString outputProfile)
{
    return p->transformTo(image, inputProfile, outputProfile);
}
