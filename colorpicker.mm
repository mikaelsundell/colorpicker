// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <QMutex>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>

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

    typedef struct
    {
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

    struct DisplayInfo {
        uint32_t displayNumber;
        CFStringRef displayProfile;
        DisplayInfo()
        {}
        ~DisplayInfo()
        {
            CFRelease(displayProfile);
        }
    };

    DisplayInfo grabDisplayInfo(NSScreen* screen)
    {
        DisplayInfo display;
        NSDictionary *deviceDescription = [screen deviceDescription];
        CGDirectDisplayID displayId = (CGDirectDisplayID)[[deviceDescription objectForKey:@"NSScreenNumber"] unsignedIntValue];
        display.displayNumber = displayId;

        ColorSync colorsync;
        colorsync.displayUUid = CGDisplayCreateUUIDFromDisplayID(displayId);
        colorsync.deviceProfileUrl = NULL;

        ColorSyncIterateDeviceProfiles(colorSyncIterateCallback, (void *)&colorsync);
        CFStringRef deviceProfileURL = CFURLCopyFileSystemPath(colorsync.deviceProfileUrl, kCFURLPOSIXPathStyle);
        CFRelease(colorsync.displayUUid);
        CFRelease(colorsync.deviceProfileUrl);
        display.displayProfile = deviceProfileURL;
        return display;
    }


    DisplayInfo
    grabDisplayInfo(NSPoint point)
    {
        for(NSScreen* screen in NSScreen.screens)
        {
            if (NSMouseInRect(point, screen.frame, false))
            {
                return grabDisplayInfo(screen);
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
    NSApplication *app = [NSApplication sharedApplication];
    [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification
                                                      object:nil
                                                       queue:nil
                                                  usingBlock:^(NSNotification *notification) {
        NSPoint point = [NSEvent mouseLocation];
        QPoint mouseLocation = convertMouseLocation(point);
        if (active()) {
            if (mouseLocation != lastpos && mutex.tryLock()) {
                DisplayInfo display = grabDisplayInfo(point);
                pickEvent(
                    Colorpicker::PickEvent() =
                    {
                        int(display.displayNumber),
                        QString::fromCFString(display.displayProfile),
                        mouseLocation
                    }
                          
                );
                lastpos = mouseLocation;
                mutex.unlock();
            }
        } else {
            moveEvent(
                Colorpicker::MoveEvent() =
                {
                    mouseLocation
                }
            );
        }
    }];
    [NSEvent addLocalMonitorForEventsMatchingMask:
       (NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged)
        handler:^(NSEvent * event)
    {
        NSPoint point = [NSEvent mouseLocation];
        QPoint mouseLocation = convertMouseLocation(point);
        if (active()) {
            if (mouseLocation != lastpos && mutex.tryLock()) {
                DisplayInfo display = grabDisplayInfo(point);
                pickEvent(
                    Colorpicker::PickEvent() =
                    {
                        int(display.displayNumber),
                        QString::fromCFString(display.displayProfile),
                        mouseLocation
                    }
                          
                );
                lastpos = mouseLocation;
                mutex.unlock();
            }
        } else {
            moveEvent(
                Colorpicker::MoveEvent() =
                {
                    mouseLocation
                }
            );
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
        if (active()) {
            if (mouseLocation != lastpos && mutex.tryLock()) {
                DisplayInfo display = grabDisplayInfo(point);
                pickEvent(
                    Colorpicker::PickEvent() =
                    {
                        int(display.displayNumber),
                        QString::fromCFString(display.displayProfile),
                        mouseLocation
                    }
                );
                lastpos = mouseLocation;
                mutex.unlock();
            }
        } else {
            moveEvent(
                Colorpicker::MoveEvent() =
                {
                    mouseLocation
                }
            );
        }
    }];
}
