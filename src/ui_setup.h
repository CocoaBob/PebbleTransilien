//
//  ui_setup.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#include <pebble.h>

// MARK: Status bar layer

void ui_setup_status_bar(Layer *window_layer, Layer **status_bar_layer);

// MARK: Theme

#ifdef PBL_COLOR
void ui_setup_theme(Window *window_layer, MenuLayer *menu_layer);
#else
void ui_setup_theme(Window *window_layer, InverterLayer *inverter_layer);
#endif