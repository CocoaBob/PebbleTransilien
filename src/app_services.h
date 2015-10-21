//
//  app_services.h
//  PebbleTransilien
//
//  Created by CocoaBob on 14/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

// MARK: AccelTapService
// Used for updating trains data

typedef void (*AccelTapServiceHandler)(AccelAxisType axis, int32_t direction, void *context);
void accel_tap_service_init(AccelTapServiceHandler handler, void *context);
void accel_tap_service_deinit();

// MARK: TickTimerService
// Used for updating the time on the status bar

typedef void (*TickTimerServiceHandler)(struct tm *tick_time, TimeUnits units_changed, void *context);
void tick_timer_service_init(TickTimerServiceHandler handler, void *context);
void tick_timer_service_deinit();