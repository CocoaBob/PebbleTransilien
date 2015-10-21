//
//  ui_status_bar.c
//  PebbleTransilien
//
//  Created by CocoaBob on 21/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static Layer *s_status_bar_layer;

// MARK: Draw status_bar

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
    graphics_context_set_text_color(ctx, fg_color);
    
    // Draw connection indicators
    bool is_connected = connection_service_peek_pebble_app_connection();
    size_t y_indicator = STATUS_BAR_LAYER_HEIGHT / 2 - 1;
    size_t x_indicator = 8; // Origin X
    size_t d_indicator = 7; // X delta
    for (int i = 0; i < 3; ++i) {
        GPoint position = GPoint(8 + 7 * i, y_indicator);
        if (is_connected) {
            graphics_fill_circle(ctx, position, 2);
        } else {
            graphics_draw_circle(ctx, position, 2);
        }
    }
    
    // Hour:Minute
    char time_buffer[16];
    clock_copy_time_string(time_buffer, sizeof(time_buffer));
    size_t y_time = (STATUS_BAR_LAYER_HEIGHT - 20) / 2;
    graphics_draw_text(ctx,
                       time_buffer,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(bounds.size.w / 2 - 30, y_time, 60, 20),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL);
    
    // Battery meter
    size_t y_battery = (STATUS_BAR_LAYER_HEIGHT - 8) / 2;
    graphics_draw_rect(ctx, GRect(bounds.size.w - 18, y_battery, 14, 8));
    graphics_draw_line(ctx, GPoint(bounds.size.w - 4, 6), GPoint(bounds.size.w - 4, 9));
    graphics_fill_rect(ctx, GRect(bounds.size.w - 16, y_battery + 2, battery_state_service_peek().charge_percent / 10, 4), 0, GCornerNone);
    
    // Separator
    graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
}

// MARK: Connection Service handler

static void connection_service_handler(bool connected) {
    status_bar_update();
}

// MARK: Get status_bar, Update status_bar

Layer *status_bar(GRect frame) {
    layer_set_frame(s_status_bar_layer, frame);
    return s_status_bar_layer;
}

void status_bar_update() {
    layer_mark_dirty(s_status_bar_layer);
}

// MARK: Setup

void status_bar_init() {
    s_status_bar_layer = layer_create(GRect(0, 0, 144, STATUS_BAR_LAYER_HEIGHT));
    layer_set_update_proc(s_status_bar_layer, status_bar_background_layer_proc);
    
    connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = connection_service_handler
    });
}

void status_bar_deinit() {
    connection_service_unsubscribe();
    
    layer_destroy(s_status_bar_layer);
}