// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "lcms2.h"
#include <QPixmap>
#include <QMap>

namespace lcms2
{
    namespace
    {
        QMap<QString, QMap<cmsUInt32Number, QMap<QString, cmsHTRANSFORM> > > cache;
    }

    cmsUInt32Number
    convertFormat(QImage::Format format)
    {
        switch (format)
        {
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

    cmsHTRANSFORM convertTransform(QString profile, cmsUInt32Number format, QString outProfile)
    {
        cmsHTRANSFORM transform = nullptr;
        if (!cache.contains(profile))
        {
            cache.insert(profile, QMap<cmsUInt32Number, QMap<QString, cmsHTRANSFORM> >());
        }
        if (!cache[profile].contains(format))
        {
            cache[profile].insert(format, QMap<QString, cmsHTRANSFORM>());
        }
        if (!cache[profile][format].contains(outProfile))
        {
            cmsHPROFILE cmsProfile = cmsOpenProfileFromFile(profile.toLocal8Bit().constData(), "r");
            cmsHPROFILE cmsDisplayProfile = cmsOpenProfileFromFile(outProfile.toLocal8Bit().constData(), "r");
            cache[profile][format].insert(
                outProfile,
                cmsCreateTransform(cmsProfile, format, cmsDisplayProfile, format, INTENT_PERCEPTUAL, 0)
            );
        }
        return cache[profile][format][outProfile];
    }

    QRgb convertColor(QRgb color, QString profile, QString outProfile)
    {
        cmsHTRANSFORM transform = convertTransform(
            profile, convertFormat(QImage::Format_RGB32), outProfile
        );
        QRgb transformColor;
        cmsDoTransform(transform, (const void*)&color, (void*)&transformColor, sizeof(QRgb));
        return transformColor;
    }

    QPixmap convertPixmap(QPixmap pixmap, QString profile, QString outProfile)
    {
        QImage image = pixmap.toImage();
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
        QPixmap transformpixmap = QPixmap::fromImage(transformimage);
        transformpixmap.setDevicePixelRatio(pixmap.devicePixelRatio());
        return transformpixmap;
    }

    void clear()
    {
        cache.clear();
    }
}
