//
//  setup_ui.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#include <pebble.h>

// MARK: Status bar and its overlay

#ifdef PBL_PLATFORM_BASALT
void status_bar_set_colors(StatusBarLayer *status_bar_layer);
void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **status_bar_background_layer, Layer **status_bar_overlay_layer);
#endif

// MARK: Theme

#ifdef PBL_COLOR
void setup_ui_theme_for_menu_layer(MenuLayer *menu_layer);
#else
void setup_ui_theme(Window *window_layer, InverterLayer *inverter_layer);
#endif