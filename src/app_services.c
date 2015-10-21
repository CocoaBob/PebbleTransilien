//
//  app_services.c
//  PebbleTransilien
//
//  Created by CocoaBob on 14/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: AccelTapService

static AccelTapServiceHandler s_accel_tap_service_handler;
static void *s_accel_tap_service_context;

static void accel_tap_service_handler(AccelAxisType axis, int32_t direction) {
    s_accel_tap_service_handler(axis, direction, s_accel_tap_service_context);
}

void accel_tap_service_init(AccelTapServiceHandler handler, void *context) {
    s_accel_tap_service_handler = handler;
    s_accel_tap_service_context = context;
    accel_tap_service_subscribe(accel_tap_service_handler);
}

void accel_tap_service_deinit() {
    accel_tap_service_unsubscribe();
    s_accel_tap_service_handler = NULL;
    s_accel_tap_service_context = NULL;
}

// MARK: TickTimerService

static TickTimerServiceHandler s_tick_timer_service_handler;
static void *s_tick_timer_service_context;

static void tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed) {
    s_tick_timer_service_handler(tick_time, units_changed, s_tick_timer_service_context);
}

void tick_timer_service_init(TickTimerServiceHandler handler, void *context) {
    s_tick_timer_service_handler = handler;
    s_tick_timer_service_context = context;
    tick_timer_service_subscribe(HOUR_UNIT | MINUTE_UNIT, tick_timer_service_handler);
}

void tick_timer_service_deinit() {
    tick_timer_service_unsubscribe();
}