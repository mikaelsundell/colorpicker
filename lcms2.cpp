// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker
#include "lcms2.h"
#include <QMap>
#include <QDebug>

namespace lcms2
{
    cmsUInt32Number
    convertFormat(QImage::Format format)
    {
        switch (format)
        {
            case QImage::Format_ARGB32:  //  (0xAARRGGBB)
            case QImage::Format_RGB32:   //  (0xffRRGGBB)
                return TYPE_BGRA_8;

            case QImage::Format_RGB888:
                return TYPE_RGB_8;       // 24-bit RGB format (8-8-8).

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

    QColor convertColor(QColor color, QString profile, QString displayProfile)
    {
        static QMap<QString, QMap<QString, cmsHTRANSFORM> > transforms;
        cmsHTRANSFORM transform = nullptr;
        if (transforms.contains(profile))
        {
            QMap<QString, cmsHTRANSFORM> profiletransforms = transforms[profile];
            if (profiletransforms.contains(displayProfile))
            {
                transform = profiletransforms[displayProfile];
            }
        }
        if (transform == nullptr)
        {
            if (!transforms.contains(profile))
            {
                transforms.insert(profile, QMap<QString, cmsHTRANSFORM>());
            }
            cmsHPROFILE cmsProfile = cmsOpenProfileFromFile(profile.toLocal8Bit().constData(), "r");
            cmsHPROFILE cmsDisplayProfile = cmsOpenProfileFromFile(displayProfile.toLocal8Bit().constData(), "r");
            transform = cmsCreateTransform
            (cmsProfile, convertFormat(QImage::Format_RGB32), cmsDisplayProfile, convertFormat(QImage::Format_RGB32), INTENT_PERCEPTUAL, 0);

            transforms[profile].insert(displayProfile, transform);
        }
        QRgb rgb = color.rgb();
        QRgb convertRgb;
        cmsDoTransform(transform, (const void*)&rgb, (void*)&convertRgb, sizeof(QRgb));
        return QColor(convertRgb);
    }
}
