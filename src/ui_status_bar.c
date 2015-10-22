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
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, bg_color);
#else
    // In Aplite, the default status bar's background is black, so we have to invert the colors by default
    graphics_context_set_fill_color(ctx, fg_color);
#endif
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Set components color
#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_context_set_fill_color(ctx, fg_color);
    graphics_context_set_text_color(ctx, fg_color);
#else
    // In Aplite, the default status bar's background is black, so we have to invert the colors by default
    graphics_context_set_stroke_color(ctx, bg_color);
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_context_set_text_color(ctx, bg_color);
#endif
    
    // Draw signal indicators
    bool is_connected = connection_service_peek_pebble_app_connection();
    size_t y_signal = (STATUS_BAR_LAYER_HEIGHT - 10) / 2;
    GRect frame_signal = GRect(4, y_signal, 11, 10);
#ifdef PBL_COLOR
    if (is_connected) {
        draw_image_in_rect(ctx, settings_is_dark_theme()?RESOURCE_ID_IMG_SIGNAL_YES_DARK:RESOURCE_ID_IMG_SIGNAL_YES_LIGHT, frame_signal);
    } else {
        draw_image_in_rect(ctx, settings_is_dark_theme()?RESOURCE_ID_IMG_SIGNAL_NO_DARK:RESOURCE_ID_IMG_SIGNAL_NO_LIGHT, frame_signal);
    }
#else
    // In Aplite, the default status bar's background is black, so we have to invert the colors by default
    if (is_connected) {
        draw_image_in_rect(ctx, true, RESOURCE_ID_IMG_SIGNAL_YES_LIGHT, frame_signal);
    } else {
        draw_image_in_rect(ctx, true, RESOURCE_ID_IMG_SIGNAL_NO_LIGHT, frame_signal);
    }
#endif
    
    // Hour:Minute
    char time_buffer[16];
    clock_copy_time_string(time_buffer, sizeof(time_buffer));
#ifdef PBL_COLOR
    size_t y_time = (STATUS_BAR_LAYER_HEIGHT - 20) / 2;
#else
    size_t y_time = (STATUS_BAR_LAYER_HEIGHT - 18) / 2;
#endif
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
#ifdef PBL_COLOR
    graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
#else
    // In Aplite, the default status bar's background is black, so we have to invert the colors by default
    // But the separator's color shouldn't
#endif
}

// MARK: Connection Service handler

static void connection_service_handler(bool connected) {
    status_bar_update();
}

// MARK: Tick Timer Service handler

static void tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed) {
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
    
    tick_timer_service_subscribe(HOUR_UNIT | MINUTE_UNIT, tick_timer_service_handler);
}

void status_bar_deinit() {
    tick_timer_service_unsubscribe();
    
    connection_service_unsubscribe();
    
    layer_destroy(s_status_bar_layer);
}