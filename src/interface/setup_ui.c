//
//  setup_ui.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Status bar and its overlay

//#define DRAW_LOGO

#ifdef PBL_PLATFORM_BASALT
static void status_bar_background_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    GColor bg_color = curr_bg_color();
    GColor fg_color = curr_fg_color();
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_context_set_stroke_color(ctx, fg_color);
    
#ifdef DRAW_LOGO
    // Fill background
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Show logo
    draw_image_in_rect(ctx, status_is_dark_theme()?RESOURCE_ID_IMG_LOGO_TRANSILIEN_DARK:RESOURCE_ID_IMG_LOGO_TRANSILIEN_LIGHT, GRect(4,3,69,10));
#else
    // Show legacy battery meter
    // Emulator battery meter on Aplite
    graphics_draw_rect(ctx, GRect(126, 4, 14, 8));
    graphics_draw_line(ctx, GPoint(140, 6), GPoint(140, 9));
    
    BatteryChargeState state = battery_state_service_peek();
    int width = (int)(float)(((float)state.charge_percent / 100.0F) * 10.0F);
    graphics_context_set_fill_color(ctx, fg_color);
    graphics_fill_rect(ctx, GRect(128, 6, width, 4), 0, GCornerNone);
#endif
    
    // Separator
    graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
}

#ifdef DRAW_LOGO
static void status_bar_overlay_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    graphics_context_set_stroke_color(ctx, curr_fg_color());
    
    // Draw SNCF style time frame
    graphics_draw_round_rect(ctx,  bounds, 3);
}
#endif

void status_bar_set_colors(StatusBarLayer *status_bar_layer) {
    status_bar_layer_set_colors(status_bar_layer, status_is_dark_theme()?GColorBlack:GColorWhite, status_is_dark_theme()?GColorWhite:GColorBlack);
}

void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **status_bar_background_layer, Layer **status_bar_overlay_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    
    // Add status bar
    *status_bar_layer = status_bar_layer_create();
#ifdef DRAW_LOGO
    GRect status_bar_frame = bounds;GRect(106, -1, 32, STATUS_BAR_LAYER_HEIGHT);
    layer_set_frame(status_bar_layer_get_layer(*status_bar_layer), status_bar_frame);
#endif
    layer_add_child(window_layer, status_bar_layer_get_layer(*status_bar_layer));
    // Setup status bar
    status_bar_set_colors(*status_bar_layer);
    
    // Status bar background
    *status_bar_background_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_set_update_proc(*status_bar_background_layer, status_bar_background_layer_proc);
    layer_add_child(window_layer, *status_bar_background_layer);
    
#ifdef DRAW_LOGO
    // Add status bar overlay
    *status_bar_overlay_layer = layer_create(GRect(status_bar_frame.origin.x, status_bar_frame.origin.y - 3, status_bar_frame.size.w, status_bar_frame.size.h + 2));
    layer_set_update_proc(*status_bar_overlay_layer, status_bar_overlay_layer_proc);
    layer_add_child(window_layer, *status_bar_overlay_layer);
#endif
}
#endif

// MARK: Theme

#ifdef PBL_COLOR
void setup_ui_theme(Window *window_layer, MenuLayer *menu_layer) {
    window_set_background_color(window_layer, curr_fg_color());
    
    menu_layer_set_normal_colors(menu_layer, curr_bg_color(), curr_fg_color());
    menu_layer_set_highlight_colors(menu_layer, GColorCobaltBlue, GColorWhite);
}
#else
void setup_ui_theme(Window *window, InverterLayer *inverter_layer) {
    if (status_is_dark_theme()) {
        Layer *window_layer = window_get_root_layer(window);
        layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
    } else {
        layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    }
}
#endif

// MARK: Deactivate menu layer

#ifdef PBL_COLOR
void set_menu_layer_activated(MenuLayer *menu_layer, bool activated) {
    if (activated) {
        menu_layer_set_highlight_colors(menu_layer, GColorCobaltBlue, GColorWhite);
    } else {
        menu_layer_set_highlight_colors(menu_layer, curr_bg_color(), curr_fg_color());
    }
}
#else
void set_menu_layer_activated(MenuLayer *menu_layer, bool activated, InverterLayer *inverter_layer_for_row) {
    Layer *layer_layer = inverter_layer_get_layer(inverter_layer_for_row);
    if (activated) {
        if (layer_get_window(layer_layer)) {
            layer_remove_from_parent(layer_layer);
        }
    } else {
        layer_add_child(menu_layer_get_layer(menu_layer), layer_layer);
    }
}
#endif
