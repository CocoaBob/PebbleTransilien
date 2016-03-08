//
//  ui_status_bar.c
//  PebbleTransilien
//
//  Created by CocoaBob on 21/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

typedef struct StatusBarData {
    AppTimer *alert_timer;
    size_t alert_count;
    char alert_text[32];
} StatusBarData;

static Layer *s_status_bar_layer;

// MARK: Draw status_bar

static void status_bar_draw_background(GContext *ctx, GRect bounds, bool is_inverted, GColor bg_color, GColor fg_color) {
    graphics_context_set_fill_color(ctx, is_inverted?fg_color:bg_color);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

static void status_bar_background_layer_proc(Layer *layer, GContext *ctx) {
    StatusBarData *user_info = layer_get_data(s_status_bar_layer);
    
    GRect bounds = layer_get_bounds(layer);
    
    GColor bg_color = curr_bg_color();
    GColor fg_color = curr_fg_color();
    
    // If alert timer != NULL, draw alert text
    if (user_info->alert_timer) {
        if (user_info->alert_count % 2 == 0) { // Invert color
            status_bar_draw_background(ctx, bounds, true, bg_color, fg_color);
            graphics_context_set_stroke_color(ctx, bg_color);
            graphics_context_set_text_color(ctx, bg_color);
        } else { // Normal color
            status_bar_draw_background(ctx, bounds, false, bg_color, fg_color);
            graphics_context_set_stroke_color(ctx, fg_color);
            graphics_context_set_text_color(ctx, fg_color);
        }
        
        // Draw frame
        graphics_draw_rect(ctx, bounds);
        
        // Draw text
        GRect frame_text = bounds;
        frame_text.origin.y -= 2;
        graphics_draw_text(ctx,
                           user_info->alert_text,
                           fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                           frame_text,
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentCenter,
                           NULL);
    } else {
        status_bar_draw_background(ctx, bounds, false, bg_color, fg_color);
        
        // Set components color
        graphics_context_set_stroke_color(ctx, fg_color);
        graphics_context_set_fill_color(ctx, fg_color);
        graphics_context_set_text_color(ctx, fg_color);
        
        // Offset
#ifdef PBL_ROUND
        int16_t offset_x = 50;
        int16_t offset_y = 4;
#else
        int16_t offset_x = 4;
        int16_t offset_y = 0;
#endif
        
        // Draw signal indicators
        bool is_connected = connection_service_peek_pebble_app_connection();
        size_t y_signal = (STATUS_BAR_LAYER_HEIGHT - 10) / 2 + offset_y;
        GRect frame_signal = GRect(offset_x, y_signal, 11, 10);
        if (is_connected) {
            draw_image_in_rect(ctx, settings_is_dark_theme()?RESOURCE_ID_IMG_SIGNAL_YES_DARK:RESOURCE_ID_IMG_SIGNAL_YES_LIGHT, frame_signal);
        } else {
            draw_image_in_rect(ctx, settings_is_dark_theme()?RESOURCE_ID_IMG_SIGNAL_NO_DARK:RESOURCE_ID_IMG_SIGNAL_NO_LIGHT, frame_signal);
        }
        
        // Battery meter
        size_t y_battery = (STATUS_BAR_LAYER_HEIGHT - 8) / 2 + offset_y;
        GRect frame_battery_body = GRect(bounds.size.w - 18 - offset_x, y_battery, 14, 8);
        graphics_draw_rect(ctx, frame_battery_body);
        graphics_draw_line(ctx, GPoint(bounds.size.w - 4 - offset_x, y_battery + 2), GPoint(bounds.size.w - 4 - offset_x, y_battery + 5));
        graphics_fill_rect(ctx, GRect(bounds.size.w - 16 - offset_x, y_battery + 2, battery_state_service_peek().charge_percent / 10, 4), 0, GCornerNone);
        
        // Hour:Minute
        char time_buffer[16];
        clock_copy_time_string(time_buffer, sizeof(time_buffer));
        size_t y_time = (STATUS_BAR_LAYER_HEIGHT - 20) / 2 + offset_y;
        GRect frame_time = GRect(frame_signal.origin.x + frame_signal.size.w,
                                 y_time,
                                 frame_battery_body.origin.x - frame_signal.origin.x - frame_signal.size.w,
                                 20);
        graphics_draw_text(ctx,
                           time_buffer,
                           fonts_get_system_font(FONT_KEY_GOTHIC_14),
                           frame_time,
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentCenter,
                           NULL);
        
        // Separator
        graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
    }
}

// MARK: Connection Service handler

static void connection_service_handler(bool connected) {
    status_bar_update();
}

// MARK: Tick Timer Service handler

static void tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed) {
    status_bar_update();
}

// MARK: Low Memory warning

static void status_bar_alert_timer_callback() {
    StatusBarData *user_info = layer_get_data(s_status_bar_layer);
    if (user_info->alert_count > 0) {
        user_info->alert_timer = app_timer_register((user_info->alert_count==1)?1000:300, // Stay 1 second for the last time
                                                 (AppTimerCallback)status_bar_alert_timer_callback,
                                                 NULL);
        user_info->alert_count -= 1;
    } else {
        user_info->alert_timer = NULL;
    }
    status_bar_update();
}

void status_bar_low_memory_alert() {
    StatusBarData *user_info = layer_get_data(s_status_bar_layer);
    if (user_info->alert_timer) {
        app_timer_cancel(user_info->alert_timer);
    }
    user_info->alert_count = 5;
    strcpy(user_info->alert_text, _("Low Memory!"));
    status_bar_alert_timer_callback();
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
    s_status_bar_layer = layer_create_with_data(GRect(0, 0, 144, STATUS_BAR_LAYER_HEIGHT), sizeof(StatusBarData));
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