//
//  status_bar.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// Status bar and its overlay

#ifdef PBL_SDK_3
static void status_bar_overlay_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    GColor color = status_is_dark_theme()?GColorWhite:GColorBlack;
    
    // Show legacy battery meter
    // Emulator battery meter on Aplite
    graphics_context_set_stroke_color(ctx, color);
    graphics_draw_rect(ctx, GRect(126, 4, 14, 8));
    graphics_draw_line(ctx, GPoint(140, 6), GPoint(140, 9));
    
    BatteryChargeState state = battery_state_service_peek();
    int width = (int)(float)(((float)state.charge_percent / 100.0F) * 10.0F);
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(128, 6, width, 4), 0, GCornerNone);
    
    // Show SNCF logo
    draw_image_in_rect(ctx, status_is_dark_theme()?RESOURCE_ID_IMG_LOGO_SNCF_DARK:RESOURCE_ID_IMG_LOGO_SNCF_LIGHT, GRect(2,2,41,11));
    
    // Separator
    graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
}

void status_bar_set_colors(StatusBarLayer *status_bar_layer) {
    status_bar_layer_set_colors(status_bar_layer, status_is_dark_theme()?GColorBlack:GColorWhite, status_is_dark_theme()?GColorWhite:GColorBlack);
}

void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **status_bar_overlay_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    // Add status bar
    *status_bar_layer = status_bar_layer_create();
    layer_add_child(window_layer, status_bar_layer_get_layer(*status_bar_layer));
    // Setup status bar
    status_bar_set_colors(*status_bar_layer);
    
    // Status bar overlay
    *status_bar_overlay_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_set_update_proc(*status_bar_overlay_layer, status_bar_overlay_layer_proc);
    layer_add_child(window_layer, *status_bar_overlay_layer);
}
#endif