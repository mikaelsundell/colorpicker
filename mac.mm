// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <QPainter>
#include <QPaintEngine>
#include <QDebug>

namespace mac
{
    namespace
    {
        void printImage(CGImageRef cgImage)
        {
            CGDataProviderRef dataProvider = CGImageGetDataProvider(cgImage);
            __block CFDataRef dataRef;
            dataRef = CGDataProviderCopyData(dataProvider);

            UInt8* buffer = (UInt8*)CFDataGetBytePtr(dataRef);
            size_t bytesPerRow = CGImageGetBytesPerRow(cgImage);

            int width = (int)CGImageGetWidth(cgImage);
            int height = (int)CGImageGetHeight(cgImage);

            for (NSInteger x=0; x<width; x++)
            {
                for (NSInteger y=0; y<height; y++)
                {
                    UInt8*  pixelPtr = buffer + (int)(y) * bytesPerRow + (int)(x) * 4;
                    UInt8 r = *(pixelPtr + 2);
                    UInt8 g = *(pixelPtr + 1);
                    UInt8 b = *(pixelPtr + 0);
                    qDebug()
                        << x
                        << ", "
                        << y
                        << ": "
                        << r
                        << ", "
                        << g
                        << ", "
                        << b
                        << "\n";
                }
            }
            CFRelease(dataRef);
        }

        void printQImage(QImage image)
        {
            int width = image.width();
            int height = image.height();
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    QColor color = image.pixel(x, y);
                    qDebug()
                        << x
                        << ", "
                        << y
                        << ": "
                        << color.red()
                        << ", "
                        << color.green()
                        << ", "
                        << color.blue()
                        << "\n";
                }
            }
        }
    
        CGBitmapInfo convertFormat(const QImage &image)
        {
            CGBitmapInfo bitmapInfo = kCGImageAlphaNone;
            switch (image.format())
            {
                case QImage::Format_ARGB32:
                    bitmapInfo = kCGImageAlphaFirst | kCGBitmapByteOrder32Host;
                    break;
                case QImage::Format_RGB32:
                    bitmapInfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
                    break;
                case QImage::Format_RGBA8888_Premultiplied:
                    bitmapInfo = kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big;
                    break;
                case QImage::Format_RGBA8888:
                    bitmapInfo = kCGImageAlphaLast | kCGBitmapByteOrder32Big;
                    break;
                case QImage::Format_RGBX8888:
                    bitmapInfo = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big;
                    break;
                case QImage::Format_ARGB32_Premultiplied:
                    bitmapInfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
                    break;
                default: break;
            }
            return bitmapInfo;
        }

        QImage convertImageToQImage(CGImageRef cgImage)
        {
            const int width = (int)CGImageGetWidth(cgImage);
            const int height = (int)CGImageGetHeight(cgImage);
            QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            // keep colorspace from incoming image ref as native values
            CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
            CGContextRef context = CGBitmapContextCreate((void *)image.bits(), image.width(), image.height(), 8,
                                                         image.bytesPerLine(), colorSpace, convertFormat(image));
            
            // scale the context so that painting happens in device-independent pixels
            const qreal devicePixelRatio = image.devicePixelRatio();
            CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
            
            CGRect rect = CGRectMake(0, 0, width, height);
            CGContextDrawImage(context, rect, cgImage);
            CGContextRelease(context);
            return image;
        }

        NSWindow* convertWId(WId winId)
        {
            NSView *view = (NSView*)winId;
            return [view window];
        }
    }

    void setupMac()
    {
        // we force dark aque no matter appearance set in system settings
        [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
    }

    void setupOverlay(WId wid)
    {
        NSView *view = reinterpret_cast<NSView *>(wid);
        NSWindow *window = [view window];
        // force window to be at status window level
        [window setLevel:NSStatusWind wLevel];
    }

    QPixmap grabDisplayPixmap(int x, int y, int width, int height)
    {
        QRect grabRect = QRect(x, y, width, height);

        // find which displays to grab from, or all of them if the grab size is unspecified
        const int maxDisplays = 128;
        CGDirectDisplayID displays[maxDisplays];
        CGDisplayCount displayCount;
        CGRect cgRect = (width < 0 || height < 0) ? CGRectInfinite : grabRect.toCGRect();
        const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);
        if (err || displayCount == 0)
            return QPixmap();

        // if the grab size is not specified, set it to be the bounding box of all screens,
        if (width < 0 || height < 0)
        {
            QRect windowRect;
            for (uint i = 0; i < displayCount; ++i)
            {
                QRect displayBounds = QRectF::fromCGRect(CGDisplayBounds(displays[i])).toRect();
                // Only include the screen if it is positioned past the x/y position
                if ((displayBounds.x() >= x || displayBounds.right() > x) &&
                    (displayBounds.y() >= y || displayBounds.bottom() > y)) {
                    windowRect = windowRect.united(displayBounds);
                }
            }
            if (grabRect.width() < 0)
                grabRect.setWidth(windowRect.width());
            if (grabRect.height() < 0)
                grabRect.setHeight(windowRect.height());
        }

        // grab images from each display
        QVector<QImage> images;
        QVector<QRect> destinations;
        for (uint i = 0; i < displayCount; ++i)
        {
            auto display = displays[i];
            QRect displayBounds = QRectF::fromCGRect(CGDisplayBounds(display)).toRect();
            QRect grabBounds = displayBounds.intersected(grabRect);
            if (grabBounds.isNull())
            {
                destinations.append(QRect());
                images.append(QImage());
                continue;
            }
            QRect displayLocalGrabBounds = QRect(QPoint(grabBounds.topLeft() - displayBounds.topLeft()), grabBounds.size());
            QImage displayImage = convertImageToQImage(CGDisplayCreateImageForRect(display, displayLocalGrabBounds.toCGRect()));
            displayImage.setDevicePixelRatio(displayImage.size().width() / displayLocalGrabBounds.size().width());
            images.append(displayImage);
            QRect destBounds = QRect(QPoint(grabBounds.topLeft() - grabRect.topLeft()), grabBounds.size());
            destinations.append(destBounds);
        }
        // determine the highest dpr, which becomes the dpr for the returned pixmap.
        qreal dpr = 1.0;
        for (uint i = 0; i < displayCount; ++i)
            dpr = qMax(dpr, images.at(i).devicePixelRatio());
        
        // allocate target pixmap and draw each screen's content
        QPixmap windowPixmap(grabRect.size() * dpr);
        windowPixmap.setDevicePixelRatio(dpr);
        windowPixmap.fill(Qt::transparent);
        QPainter painter(&windowPixmap);
        for (uint i = 0; i < displayCount; ++i)
            painter.drawImage(destinations.at(i), images.at(i));
        return windowPixmap;
    }

    QPixmap grabDisplayPixmap(int x, int y, int width, int height, WId excludeId)
    {
        CGWindowID windowIDToExcude = (CGWindowID)[convertWId(excludeId) windowNumber];
        QRect grabRect = QRect(x, y, width, height);
        
        CFArrayRef screenWindows = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
        CFMutableArrayRef grabWindows = CFArrayCreateMutableCopy(NULL, 0, screenWindows);
        for (long i = CFArrayGetCount(grabWindows) - 1; i >= 0; i--)
        {
            CGWindowID window = (CGWindowID)(uintptr_t)CFArrayGetValueAtIndex(grabWindows, i);
            if (window == windowIDToExcude)
                CFArrayRemoveValueAtIndex(grabWindows, i);
        }
        
        // find which displays to grab from, or all of them if the grab size is unspecified
        const int maxDisplays = 128;
        CGDirectDisplayID displays[maxDisplays];
        CGDisplayCount displayCount;
        CGRect cgRect = (width < 0 || height < 0) ? CGRectInfinite : grabRect.toCGRect();
        const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);
        if (err || displayCount == 0)
            return QPixmap();

        // if the grab size is not specified, set it to be the bounding box of all screens,
        if (width < 0 || height < 0) {
            QRect windowRect;
            for (uint i = 0; i < displayCount; ++i) {
                QRect displayBounds = QRectF::fromCGRect(CGDisplayBounds(displays[i])).toRect();
                // Only include the screen if it is positioned past the x/y position
                if ((displayBounds.x() >= x || displayBounds.right() > x) &&
                    (displayBounds.y() >= y || displayBounds.bottom() > y)) {
                    windowRect = windowRect.united(displayBounds);
                }
            }
            if (grabRect.width() < 0)
                grabRect.setWidth(windowRect.width());
            if (grabRect.height() < 0)
                grabRect.setHeight(windowRect.height());
        }

        // grab images from each display
        QVector<QImage> images;
        QVector<QRect> destinations;
        for (uint i = 0; i < displayCount; ++i)
        {
            auto display = displays[i];
            QRect displayBounds = QRectF::fromCGRect(CGDisplayBounds(display)).toRect();
            QRect grabBounds = displayBounds.intersected(grabRect);
            if (grabBounds.isNull())
            {
                destinations.append(QRect());
                images.append(QImage());
                continue;
            }
            QImage displayImage = convertImageToQImage(CGWindowListCreateImageFromArray(grabBounds.toCGRect(), grabWindows, kCGWindowListOptionAll));
            displayImage.setDevicePixelRatio(displayImage.size().width() / grabBounds.size().width());
            images.append(displayImage);
            QRect destBounds = QRect(QPoint(grabBounds.topLeft() - grabRect.topLeft()), grabBounds.size());
            destinations.append(destBounds);
        }
        // determine the highest dpr, which becomes the dpr for the returned pixmap.
        qreal dpr = 1.0;
        for (uint i = 0; i < displayCount; ++i)
            dpr = qMax(dpr, images.at(i).devicePixelRatio());
        
        // allocate target pixmap and draw each screen's content
        QPixmap windowPixmap(grabRect.size() * dpr);
        windowPixmap.setDevicePixelRatio(dpr);
        windowPixmap.fill(Qt::transparent);
        QPainter painter(&windowPixmap);
        for (uint i = 0; i < displayCount; ++i)
            painter.drawImage(destinations.at(i), images.at(i));
        
        CFRelease(screenWindows);
        CFRelease(grabWindows);
        return windowPixmap;
    }

    typedef struct {
        CFUUIDRef displayUUid;
        CFURLRef iccProfileUrl;
    } ColorSync;

    bool
    colorSyncIterateCallback(CFDictionaryRef dict, void *data)
    {
        ColorSync* colorsync = (ColorSync *)data;
        CFStringRef str;
        CFUUIDRef uuid;
        CFBooleanRef iscur;
        if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceClass, (const void**)&str))
        {
            return true;
        }
        if (!CFEqual(str, kColorSyncDisplayDeviceClass))
        {
            return true;
        }
        if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceID, (const void**)&uuid))
        {
            return true;
        }
        if (!CFEqual(uuid, colorsync->displayUUid))
        {
            return true;
        }
        if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceProfileIsCurrent, (const void**)&iscur))
        {
            return true;
        }
        if (!CFBooleanGetValue(iscur))
        {
            return true;
        }
        if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceProfileURL, (const void**)&(colorsync->iccProfileUrl)))
        {
            return true;
        }
        CFRetain(colorsync->iccProfileUrl);
        return false;
    }

    QString grabIccProfile(WId wid)
    {
        CGWindowID windowId = (CGWindowID)[convertWId(wid) windowNumber];
        CFArrayRef screenWindow = CGWindowListCreate(kCGWindowListOptionIncludingWindow, windowId);
        CFArrayRef screenDescriptions = CGWindowListCreateDescriptionFromArray(screenWindow);
        CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex ((CFArrayRef)screenDescriptions, 0);
        // grab window bounds
        CGRect rect;
        CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)CFDictionaryGetValue(windowInfo, kCGWindowBounds), &rect);
        for(NSScreen* screen in NSScreen.screens)
        {
            if (NSMouseInRect(rect.origin, screen.frame, false))
            {
                NSDictionary* deviceDescription = [screen deviceDescription];
                CGDirectDisplayID displayId = (CGDirectDisplayID)[[deviceDescription objectForKey:@"NSScreenNumber"] unsignedIntValue];
                // colorsync callnack for device profile id
                ColorSync colorsync;
                colorsync.displayUUid = CGDisplayCreateUUIDFromDisplayID(displayId);
                colorsync.iccProfileUrl = NULL;
                ColorSyncIterateDeviceProfiles(colorSyncIterateCallback, (void *)&colorsync);
                CFRelease(colorsync.displayUUid);
                CFStringRef iccprofileurl = CFURLCopyFileSystemPath(colorsync.iccProfileUrl, kCFURLPOSIXPathStyle);
                CFRelease(colorsync.iccProfileUrl);
                QString cfstring = QString::fromCFString(iccprofileurl);
                CFRelease(iccprofileurl);
                return cfstring;
            }
        }
        CFRelease(windowInfo);
        CFRelease(screenDescriptions);
        CFRelease(screenWindow);
        return QString();
    }
}

