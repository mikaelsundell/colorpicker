// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <QPainter>
#include <QPaintEngine>
#include <QScreen>
#include <QGuiApplication>

#include <QDebug>

namespace mac
{
    namespace utils
    {
        CGBitmapInfo toNativeFormat(const QImage &image)
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

        QImage fromNativeImage(CGImageRef cgImage)
        {
            const int width = (int)CGImageGetWidth(cgImage);
            const int height = (int)CGImageGetHeight(cgImage);
            QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            // keep colorspace from incoming image ref as native values
            CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
            CGContextRef context = CGBitmapContextCreate((void *)image.bits(), image.width(), image.height(), 8,
                                                         image.bytesPerLine(), colorSpace, toNativeFormat(image));
            
            // scale the context so that painting happens in device-independent pixels
            const qreal devicePixelRatio = image.devicePixelRatio();
            CGContextScaleCTM(context, devicePixelRatio, devicePixelRatio);
            
            CGRect rect = CGRectMake(0, 0, width, height);
            CGContextDrawImage(context, rect, cgImage);
            CGContextRelease(context);
            return image;
        }

        NSWindow* toNativeWindow(WId winId)
        {
            NSView *view = (NSView*)winId;
            return [view window];
        }
    
        NSScreen* toNativeScreen(WId winId)
        {
            NSWindow *window = toNativeWindow(winId);
            return [window screen];
        }
    
        struct ColorSyncProfile {
            uint32_t screenNumber;
            CFStringRef displayProfileUrl;
            ColorSyncProfile() : screenNumber(0), displayProfileUrl(nullptr) {}
            ColorSyncProfile(const ColorSyncProfile& other)
            : screenNumber(other.screenNumber) {
                displayProfileUrl = other.displayProfileUrl ? static_cast<CFStringRef>(CFRetain(other.displayProfileUrl)) : nullptr;
            }
            ColorSyncProfile& operator=(const ColorSyncProfile& other) {
                if (this != &other) {
                    screenNumber = other.screenNumber;
                    if (displayProfileUrl) CFRelease(displayProfileUrl);
                    displayProfileUrl = other.displayProfileUrl ? static_cast<CFStringRef>(CFRetain(other.displayProfileUrl)) : nullptr;
                }
                return *this;
            }
            ~ColorSyncProfile() {
                if (displayProfileUrl) CFRelease(displayProfileUrl);
            }
        };
        QMap<uint32_t, ColorSyncProfile> colorsynccache;    
        ColorSyncProfile grabColorSyncProfile(NSScreen* screen)
        {
            ColorSyncProfile colorSyncProfile;
            NSDictionary* screenDescription = [screen deviceDescription];
            NSNumber* screenID = [screenDescription objectForKey:@"NSScreenNumber"];
            colorSyncProfile.screenNumber = [screenID unsignedIntValue];
            ColorSyncProfileRef csProfileRef = ColorSyncProfileCreateWithDisplayID((CGDirectDisplayID)colorSyncProfile.screenNumber);
            if (csProfileRef) {
                CFURLRef iccURLRef = ColorSyncProfileGetURL(csProfileRef, NULL);
                if (iccURLRef) {
                    colorSyncProfile.displayProfileUrl = CFURLCopyFileSystemPath(iccURLRef, kCFURLPOSIXPathStyle);
                }
            }
            return colorSyncProfile;
        }
    
        ColorSyncProfile grabDisplayProfile(NSScreen* screen) {
            NSDictionary* screenDescription = [screen deviceDescription];
            CGDirectDisplayID displayId = [[screenDescription objectForKey:@"NSScreenNumber"] unsignedIntValue];
            if (colorsynccache.contains(displayId)) {
                return colorsynccache.value(displayId);
            }
            ColorSyncProfile colorSyncProfile = grabColorSyncProfile(screen);
            colorsynccache.insert(displayId, colorSyncProfile);
            return colorSyncProfile;
        }
    }

    void setDarkAppearance()
    {
        // we force dark aque no matter appearance set in system settings
        [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
    }

    void setTopLevel(WId wid)
    {
        NSView *view = reinterpret_cast<NSView *>(wid);
        NSWindow *window = [view window];
        // force window to be at status window level
        [window setLevel:NSStatusWindowLevel];
    }

    QImage grabImage(int x, int y, int width, int height, WId windowId)
    {
        CGWindowID windowIdToExcude = 0;
        if (windowId != 0) {
            windowIdToExcude = (CGWindowID)[utils::toNativeWindow(windowId) windowNumber];
        }
        QRect grabRect = QRect(x, y, width, height);
        CFArrayRef screenWindows = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
        CFMutableArrayRef grabWindows = CFArrayCreateMutableCopy(NULL, 0, screenWindows);
        if (windowIdToExcude != 0) {
            for (long i = CFArrayGetCount(grabWindows) - 1; i >= 0; i--)
            {
                CGWindowID window = (CGWindowID)(uintptr_t)CFArrayGetValueAtIndex(grabWindows, i);
                if (window == windowIdToExcude)
                    CFArrayRemoveValueAtIndex(grabWindows, i);
            }
        }
        // find which displays to grab from, or all of them if the grab size is unspecified
        const int maxDisplays = 128;
        CGDirectDisplayID displays[maxDisplays];
        CGDisplayCount displayCount;
        CGRect cgRect = (width < 0 || height < 0) ? CGRectInfinite : grabRect.toCGRect();
        const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);
        if (err || displayCount == 0)
            return QImage();

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
            CGImageRef cgImage = CGWindowListCreateImageFromArray(grabBounds.toCGRect(), grabWindows, kCGWindowListOptionAll);
            QImage displayImage = utils::fromNativeImage(cgImage);
            CGImageRelease(cgImage); // Release the CGImageRef
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
        QImage windowImage(grabRect.size() * dpr, QImage::Format_ARGB32_Premultiplied);
        windowImage.setDevicePixelRatio(dpr);
        windowImage.fill(Qt::transparent); // Ensure the image background is transparent

        QPainter painter(&windowImage);
        for (uint i = 0; i < displayCount; ++i) {
            painter.drawImage(destinations.at(i), images.at(i));
        }
        painter.end(); // Ensure painting is finished properly
        CFRelease(screenWindows);
        CFRelease(grabWindows);
        return windowImage;
    }

    IccProfile grabIccProfile(int x, int y)
    {
        QPointF cursor = toNativeCursor(x, y);
        NSPoint point = NSMakePoint(cursor.x(), cursor.y());
        for(NSScreen* screen in NSScreen.screens) {
            if (NSMouseInRect(point, screen.frame, false)) {
                
                utils::ColorSyncProfile colorsyncProfile = utils::grabDisplayProfile(screen);
                return IccProfile {
                    int(colorsyncProfile.screenNumber),
                    QString::fromCFString(colorsyncProfile.displayProfileUrl)
                };
            }
        }
        return IccProfile();
    }

    IccProfile grabIccProfile(WId wid)
    {
        NSScreen* screen = utils::toNativeScreen(wid);
        utils::ColorSyncProfile colorsyncProfile = utils::grabDisplayProfile(screen);
        return IccProfile {
            int(colorsyncProfile.screenNumber),
            QString::fromCFString(colorsyncProfile.displayProfileUrl)
        };
    }

    QString grabIccProfileUrl(WId wid)
    {
        return grabIccProfile(wid).displayProfileUrl;
    }

    QPoint fromNativeCursor(float x, float y)
    {
        QPointF point(x, y);
        QScreen* screen = QGuiApplication::primaryScreen();
        // this is worth to mention, mac uses float mouse location vs. integer in Qt,
        // type conversion to closest integer will fail geometry contains() checks as
        // mouse may be rounded up and fall on edge, hence treated as outside.
        QPointF cursor = QPointF(point.x(), screen->geometry().height() - point.y());
        return QPoint(floor(cursor.x()), floor(cursor.y()));
    }

    QPointF toNativeCursor(int x, int y)
    {
        QScreen* screen = QGuiApplication::primaryScreen();
        QPointF cursor = QPointF(x, y);
        qreal reverse = screen->geometry().height() - cursor.y();
        return QPointF(cursor.x(), reverse);
    }
}

