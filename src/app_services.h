//
//  app_services.h
//  PebbleTransilien
//
//  Created by CocoaBob on 14/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

typedef void (*AccelTapServiceHandler)(AccelAxisType axis, int32_t direction, void *context);
void accel_tap_service_init(AccelTapServiceHandler handler, void *context);
void accel_tap_service_deinit();