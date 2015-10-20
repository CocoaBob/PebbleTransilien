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

static void status_bar_background_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    GColor bg_color = curr_bg_color();
    GColor fg_color = curr_fg_color();
    
    // Fill background color
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Set components color
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_context_set_fill_color(ctx, fg_color);
    
    // Battery meter
    size_t y = (STATUS_BAR_LAYER_HEIGHT - 8) / 2;
    graphics_draw_rect(ctx, GRect(bounds.size.w - 18, y, 14, 8));
    graphics_draw_line(ctx, GPoint(bounds.size.w - 4, 6), GPoint(bounds.size.w - 4, 9));
    graphics_fill_rect(ctx, GRect(bounds.size.w - 16, y + 2, battery_state_service_peek().charge_percent / 10, 4), 0, GCornerNone);
    
    // Separator
    graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
}

void window_add_status_bar(Layer *window_layer, Layer **status_bar_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    *status_bar_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_set_update_proc(*status_bar_layer, status_bar_background_layer_proc);
    layer_add_child(window_layer, *status_bar_layer);
}

// MARK: Theme

#ifdef PBL_COLOR
void ui_setup_theme(Window *window_layer, MenuLayer *menu_layer) {
    menu_layer_set_normal_colors(menu_layer, curr_bg_color(), curr_fg_color());
    menu_layer_set_highlight_colors(menu_layer, GColorCobaltBlue, GColorWhite);
}
#else
void ui_setup_theme(Window *window, InverterLayer *inverter_layer) {
    if (settings_is_dark_theme()) {
        Layer *window_layer = window_get_root_layer(window);
        layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
    } else {
        layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    }
}
#endif
