// Copyright 2022-present Contributors to the vectorscope project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorman

#include "icctransform.h"
#include <QApplication>
#include <QColorSpace>
#include <QMap>
#include <QMutex>
#include <QPointer>

QScopedPointer<ICCTransform, ICCTransform::Deleter> ICCTransform::pi;

class ICCTransformPrivate : public QObject {
    Q_OBJECT
public:
    ICCTransformPrivate();
    ~ICCTransformPrivate();
    cmsUInt32Number mapFormat(QImage::Format format);
    cmsHTRANSFORM mapTransform(const QString& profile, const QString& outProfile, QImage::Format format);
    cmsHTRANSFORM mapTransform(const QColorSpace& colorSpace, const QString& outProfile, QImage::Format format);
    QImage mapImage(QImage image, cmsHTRANSFORM transform);
    QRgb map(QRgb color, const QString& profile, const QString& outProfile);
    QImage map(QImage image, const QString& profile, const QString& outProfile);
    QImage map(QImage image, const QColorSpace& colorSpace, const QString& outProfile);

public:
    QString inputProfile;
    QString outputProfile;
    QMap<QString, QMap<QImage::Format, QMap<QString, cmsHTRANSFORM>>> cache;
    QPointer<ICCTransform> transform;
};

ICCTransformPrivate::ICCTransformPrivate() {}

ICCTransformPrivate::~ICCTransformPrivate()
{
    for (QString profile : cache.keys()) {
        for (QImage::Format format : cache[profile].keys()) {
            for (QString outputProfile : cache[profile][format].keys()) {
                cmsDeleteTransform(cache[profile][format][outputProfile]);
            }
        }
    }
    cache.clear();
}

cmsUInt32Number
ICCTransformPrivate::mapFormat(QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGB32: return TYPE_BGRA_8;

    case QImage::Format_RGB888: return TYPE_RGB_8;

    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888: return TYPE_RGBA_8;

    case QImage::Format_Grayscale8: return TYPE_GRAY_8;

    case QImage::Format_Grayscale16: return TYPE_GRAY_16;

    case QImage::Format_RGBA64:
    case QImage::Format_RGBX64: return TYPE_RGBA_16;

    case QImage::Format_BGR888: return TYPE_BGR_8;

    default: return 0;
    }
}

cmsHTRANSFORM
ICCTransformPrivate::mapTransform(const QString& profile, const QString& outProfile, QImage::Format format)
{
    if (!cache.contains(profile)) {
        cache.insert(profile, QMap<QImage::Format, QMap<QString, cmsHTRANSFORM>>());
    }
    if (!cache[profile].contains(format)) {
        cache[profile].insert(format, QMap<QString, cmsHTRANSFORM>());
    }
    if (!cache[profile][format].contains(outProfile)) {
        cmsHPROFILE cmsProfile = cmsOpenProfileFromFile(profile.toLocal8Bit().constData(), "r");
        cmsHPROFILE cmsDisplayProfile = cmsOpenProfileFromFile(outProfile.toLocal8Bit().constData(), "r");
        int flags = (format == QImage::Format_ARGB32_Premultiplied ? cmsFLAGS_COPY_ALPHA : 0);
        cache[profile][format].insert(outProfile, cmsCreateTransform(cmsProfile, mapFormat(format), cmsDisplayProfile,
                                                                     mapFormat(format), INTENT_PERCEPTUAL, flags));
        cmsCloseProfile(cmsProfile);
        cmsCloseProfile(cmsDisplayProfile);
    }
    return cache[profile][format][outProfile];
}

cmsHTRANSFORM
ICCTransformPrivate::mapTransform(const QColorSpace& colorSpace, const QString& outProfile, QImage::Format format)
{
    QString profile = colorSpace.description();
    QByteArray data = colorSpace.iccProfile();
    if (!cache.contains(profile)) {
        cache.insert(profile, QMap<QImage::Format, QMap<QString, cmsHTRANSFORM>>());
    }
    if (!cache[profile].contains(format)) {
        cache[profile].insert(format, QMap<QString, cmsHTRANSFORM>());
    }
    if (!cache[profile][format].contains(outProfile)) {
        cmsHPROFILE cmsProfile = cmsOpenProfileFromMem(data.constData(), static_cast<cmsUInt32Number>(data.size()));
        cmsHPROFILE cmsDisplayProfile = cmsOpenProfileFromFile(outProfile.toLocal8Bit().constData(), "r");
        int flags = (format == QImage::Format_ARGB32_Premultiplied ? cmsFLAGS_COPY_ALPHA : 0);
        cache[profile][format].insert(outProfile, cmsCreateTransform(cmsProfile, mapFormat(format), cmsDisplayProfile,
                                                                     mapFormat(format), INTENT_PERCEPTUAL, flags));
        cmsCloseProfile(cmsProfile);
        cmsCloseProfile(cmsDisplayProfile);
    }
    return cache[profile][format][outProfile];
}

QImage
ICCTransformPrivate::mapImage(QImage image, cmsHTRANSFORM transform)
{
    QImage mapped(image.width(), image.height(), image.format());
    cmsDoTransformLineStride(transform, image.constBits(), mapped.bits(), image.width(), image.height(),
                             static_cast<cmsUInt32Number>(image.bytesPerLine()),
                             static_cast<cmsUInt32Number>(mapped.bytesPerLine()), 0, 0);
    mapped.setDevicePixelRatio(image.devicePixelRatio());
    return mapped;
}

QRgb
ICCTransformPrivate::map(QRgb color, const QString& profile, const QString& outProfile)
{
    cmsHTRANSFORM transform = mapTransform(profile, outProfile, QImage::Format_RGB32);
    QRgb transformColor;
    cmsDoTransform(transform, &color, &transformColor, 1);
    return transformColor;
}

QImage
ICCTransformPrivate::map(QImage image, const QString& profile, const QString& outProfile)
{
    cmsHTRANSFORM transform = mapTransform(profile, outProfile, image.format());
    return mapImage(image, transform);
}

QImage
ICCTransformPrivate::map(QImage image, const QColorSpace& colorSpace, const QString& outProfile)
{
    cmsHTRANSFORM transform = mapTransform(colorSpace, outProfile, image.format());
    return mapImage(image, transform);
}

#include "icctransform.moc"

ICCTransform::ICCTransform()
    : p(new ICCTransformPrivate())
{}

ICCTransform::~ICCTransform() { p->cache.clear(); }

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
    Q_ASSERT(!p->inputProfile.isEmpty());
    return p->inputProfile;
}

void
ICCTransform::ICCTransform::setInputProfile(const QString& inputProfile)
{
    p->inputProfile = inputProfile;
    inputProfileChanged(inputProfile);
}

QString
ICCTransform::ICCTransform::outputProfile() const
{
    Q_ASSERT(!p->outputProfile.isEmpty());
    return p->outputProfile;
}

void
ICCTransform::ICCTransform::setOutputProfile(const QString& outputProfile)
{
    p->outputProfile = outputProfile;
    outputProfileChanged(outputProfile);
}

QRgb
ICCTransform::ICCTransform::map(QRgb color)
{
    return p->map(color, p->inputProfile, p->outputProfile);
}

QImage
ICCTransform::ICCTransform::map(const QImage& image)
{
    return p->map(image, p->inputProfile, p->outputProfile);
}

QRgb
ICCTransform::ICCTransform::map(QRgb color, const QString& inputProfile, const QString& outputProfile)
{
    return p->map(color, inputProfile, outputProfile);
}

QImage
ICCTransform::map(const QImage& image, const QString& inputProfile, const QString& outputProfile)
{
    return p->map(image, inputProfile, outputProfile);
}

QRgb
ICCTransform::map(QRgb color, const QColorSpace& colorSpace, const QString& outputProfile)
{
    return QRgb();
}

QImage
ICCTransform::map(const QImage& image, const QColorSpace& colorSpace, const QString& outputProfile)
{
    return p->map(image, colorSpace, outputProfile);
}
