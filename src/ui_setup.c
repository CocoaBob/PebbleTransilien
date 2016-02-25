//
//  ui_setup.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Status bar layer

void ui_setup_status_bar(Layer *window_layer, Layer *sibling_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    Layer *status_bar_layer = status_bar(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_insert_below_sibling(status_bar_layer, sibling_layer);
}

// MARK: Theme

void ui_setup_theme(Window *window_layer, MenuLayer *menu_layer) {
    menu_layer_set_normal_colors(menu_layer, curr_bg_color(), curr_fg_color());
    menu_layer_set_highlight_colors(menu_layer, PBL_IF_COLOR_ELSE(HIGHLIGHT_COLOR, curr_fg_color()), PBL_IF_COLOR_ELSE(GColorWhite, curr_bg_color()));
}