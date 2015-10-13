//
//  ui_setup.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#include <pebble.h>

// MARK: Status bar and its overlay

#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
void status_bar_set_colors(StatusBarLayer *status_bar_layer);
void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **status_bar_background_layer);
#endif

// MARK: Theme

#ifdef PBL_COLOR
void ui_setup_theme(Window *window_layer, MenuLayer *menu_layer);
#else
void ui_setup_theme(Window *window_layer, InverterLayer *inverter_layer);
#endif

#ifdef PBL_COLOR
void set_menu_layer_activated(MenuLayer *menu_layer, bool activated);
#else
void set_menu_layer_activated(MenuLayer *menu_layer, bool activated, InverterLayer *inverter_layer_for_row);
#endif