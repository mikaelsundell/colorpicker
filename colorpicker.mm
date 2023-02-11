// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

namespace
{
    QPoint convertMouseLocation(NSPoint point)
    {
        QScreen* screen = QGuiApplication::primaryScreen();
        // this is worth to mention, mac uses float mouse location vs. integer in Qt,
        // type conversion to closest integer will fail geometry contains() checks as
        // mouse may be rounded up and fall on edge, hence treated as outside.
        QPointF cursor = QPointF(point.x, screen->geometry().height() - point.y);
        return QPoint(floor(cursor.x()), floor(cursor.y()));
    }

    typedef struct {
        CFUUIDRef displayUUid;
        CFURLRef deviceProfileUrl;
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
        if (!CFDictionaryGetValueIfPresent(dict, kColorSyncDeviceProfileURL, (const void**)&(colorsync->deviceProfileUrl)))
        {
            return true;
        }
        CFRetain(colorsync->deviceProfileUrl);
        return false;
    }

    typedef struct {
        CFStringRef displayProfile;
        uint32_t displayNumber;
    } DisplayInfo;

    DisplayInfo
    grabDisplayInfo(NSPoint point)
    {
        for(NSScreen* screen in NSScreen.screens)
        {
            if (NSMouseInRect(point, screen.frame, false))
            {
                DisplayInfo display;
                NSDictionary *deviceDescription = [screen deviceDescription];
                CGDirectDisplayID displayId = (CGDirectDisplayID)[[deviceDescription objectForKey:@"NSScreenNumber"] unsignedIntValue];
                display.displayNumber = displayId;
                // colorsync callnack for device profile id
                ColorSync colorsync;
                colorsync.displayUUid = CGDisplayCreateUUIDFromDisplayID(displayId);
                colorsync.deviceProfileUrl = NULL;
                ColorSyncIterateDeviceProfiles(colorSyncIterateCallback, (void *)&colorsync);
                CFRelease(colorsync.displayUUid);
                CFStringRef deviceprofileurl = CFURLCopyFileSystemPath(colorsync.deviceProfileUrl, kCFURLPOSIXPathStyle);
                CFRelease(colorsync.deviceProfileUrl);
                display.displayProfile = deviceprofileurl;
                return display;
            }
        }
        return DisplayInfo();
    }
};

void
Colorpicker::registerEvents()
{
    static QMutex mutex;
    static QPoint lastpos;
    [NSEvent addLocalMonitorForEventsMatchingMask:
       (NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged)
        handler:^(NSEvent * event)
    {
        NSPoint point = [NSEvent mouseLocation];
        QPoint mouseLocation = convertMouseLocation(point);
        if (mouseLocation != lastpos && mutex.tryLock())
        {
            DisplayInfo display = grabDisplayInfo(point);
            updateEvents(
                Colorpicker::DisplayEvent() =
                {
                    QString::fromCFString(display.displayProfile),
                    int(display.displayNumber),
                    mouseLocation
                }
            );
            lastpos = mouseLocation;
            mutex.unlock();

        }
        return event;
    }];
    // copies of events the system posts to other applications.
    [NSEvent addGlobalMonitorForEventsMatchingMask:
     (NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged)
        handler:^(NSEvent * event)
     {
        NSPoint point = [NSEvent mouseLocation];
        QPoint mouseLocation = convertMouseLocation(point);
        if (mouseLocation != lastpos && mutex.tryLock())
        {
            DisplayInfo display = grabDisplayInfo(point);
            updateEvents(
                Colorpicker::DisplayEvent() =
                {
                    QString::fromCFString(display.displayProfile),
                    int(display.displayNumber),
                    mouseLocation
                }
            );
            lastpos = mouseLocation;
            mutex.unlock();
        }
    }];
}
