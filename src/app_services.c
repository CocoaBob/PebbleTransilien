//
//  app_services.c
//  PebbleTransilien
//
//  Created by CocoaBob on 14/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Forward declarations

static void tap_handler(AccelAxisType axis, int32_t direction);

// MARK: Init/Deinit

void services_init() {
    accel_tap_service_subscribe(tap_handler);
}

void services_deinit() {
    accel_tap_service_unsubscribe();
}

// MARK: AccelTapService

static void tap_handler(AccelAxisType axis, int32_t direction) {
    printf("%s",__func__);
}