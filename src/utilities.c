//
//  utilities.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "utilities.h"

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment) {
    graphics_draw_text(ctx, text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

// Status bar and its battery meter
#ifdef PBL_SDK_3
static void battery_proc(Layer *layer, GContext *ctx) {
    // Emulator battery meter on Aplite
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, GRect(126, 4, 14, 8));
    graphics_draw_line(ctx, GPoint(140, 6), GPoint(140, 9));
    
    BatteryChargeState state = battery_state_service_peek();
    int width = (int)(float)(((float)state.charge_percent / 100.0F) * 10.0F);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(128, 6, width, 4), 0, GCornerNone);
}

void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **battery_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    // Add status bar
    *status_bar_layer = status_bar_layer_create();
    layer_add_child(window_layer, status_bar_layer_get_layer(*status_bar_layer));
    // Setup status bar
    //   status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeDotted);
    status_bar_layer_set_colors(*status_bar_layer, GColorBlack, GColorWhite);
    // Show legacy battery meter
    *battery_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_set_update_proc(*battery_layer, battery_proc);
    layer_add_child(window_layer, *battery_layer);
}
#endif

// Draw header and separators

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(2,
                        -CELL_TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN * 2,
                        bounds.size.h);
    
    draw_text(ctx, title, FONT_KEY_GOTHIC_14_BOLD, frame, GTextAlignmentLeft);
}

void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color) {
    graphics_context_set_stroke_color(ctx, color);
    for (int16_t dx = 0; dx < layer_get_bounds(cell_layer).size.w; dx+=2) {
        graphics_draw_pixel(ctx, GPoint(dx, 0));
    }
}

// Settings

#define SETTING_THEME 100

// true is dark theme, false is light theme
bool get_setting_theme() {
    return persist_read_bool(SETTING_THEME);
}

void set_setting_theme(bool is_dark) {
    persist_write_bool(SETTING_THEME, is_dark);
}

GColor curr_fg_color() {
    return get_setting_theme()?GColorWhite:GColorBlack;
}

GColor curr_bg_color() {
    return get_setting_theme()?GColorBlack:GColorWhite;
}