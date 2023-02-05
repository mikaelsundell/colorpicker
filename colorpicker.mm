// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#include "colorpicker.h"
#include "mac.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

void
Colorpicker::registerEvents()
{
    [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskMouseMoved)
        handler:^(NSEvent * event)
    {
        NSPoint point = [NSEvent mouseLocation];
        mac::DisplayInfo display = mac::grabDisplayInfo(point.x, point.y);
        updateEvents(
            Colorpicker::DisplayEvent() =
            {
                QString::fromCFString(display.displayProfile),
                int(display.displayNumber),
                int(point.x),
                int(point.y)
            }
        );
        return event;
    }];
    // copies of events the system posts to other applications.
    [NSEvent addGlobalMonitorForEventsMatchingMask:(NSEventMaskMouseMoved)
                                           handler:^(NSEvent * event)
     {
        NSPoint point = [NSEvent mouseLocation];
        mac::DisplayInfo display = mac::grabDisplayInfo(point.x, point.y);
        updateEvents(
            Colorpicker::DisplayEvent() =
            {
                QString::fromCFString(display.displayProfile),
                int(display.displayNumber),
                int(point.x),
                int(point.y)
            }
        );
    }];
}
