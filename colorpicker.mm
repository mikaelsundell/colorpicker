// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#include <QMutex>

void
Colorpicker::registerEvents()
{
    static QMutex mutex;
    static QPoint lastpos;
    [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification
                                                      object:nil
                                                       queue:nil
                                                  usingBlock:^(NSNotification *notification) {
        NSPoint point = [NSEvent mouseLocation];
        QPoint cursor = mac::fromNativeCursor(point.x, point.y);
        if (active()) {
            if (cursor != lastpos && mutex.tryLock()) {
                mac::IccProfile iccProfile = mac::grabIccProfile(cursor.x(), cursor.y());
                pickEvent(
                    Colorpicker::PickEvent() = {
                        iccProfile.screenNumber,
                        iccProfile.displayProfileUrl,
                        cursor
                    }
                );
                lastpos = cursor;
                mutex.unlock();
            }
        } else {
            moveEvent(
                Colorpicker::MoveEvent() = {
                    cursor
                }
            );
        }
    }];
    
    [NSEvent addLocalMonitorForEventsMatchingMask:
       (NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged)
        handler:^(NSEvent * event) {
        NSPoint point = [NSEvent mouseLocation];
        QPoint cursor = mac::fromNativeCursor(point.x, point.y);
        if (active()) {
            if (cursor != lastpos && mutex.tryLock()) {
                mac::IccProfile iccProfile = mac::grabIccProfile(cursor.x(), cursor.y());
                pickEvent(
                    Colorpicker::PickEvent() = {
                        iccProfile.screenNumber,
                        iccProfile.displayProfileUrl,
                        cursor
                    }
                );
                lastpos = cursor;
                mutex.unlock();
            }
        } else {
            moveEvent(
                Colorpicker::MoveEvent() = {
                    cursor
                }
            );
        }
        return event;
    }];
    
    // copies of events the system posts to other applications.
    [NSEvent addGlobalMonitorForEventsMatchingMask:
     (NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged)
        handler:^(NSEvent * event) {
        NSPoint point = [NSEvent mouseLocation];
        QPoint cursor = mac::fromNativeCursor(point.x, point.y);
        if (active()) {
            if (cursor != lastpos && mutex.tryLock()) {
                mac::IccProfile iccProfile = mac::grabIccProfile(cursor.x(), cursor.y());
                pickEvent(
                    Colorpicker::PickEvent() = {
                        iccProfile.screenNumber,
                        iccProfile.displayProfileUrl,
                        cursor
                    }
                );
                lastpos = cursor;
                mutex.unlock();
            }
        } else {
            moveEvent(
                Colorpicker::MoveEvent() = {
                    cursor
                }
            );
        }
    }];
}
