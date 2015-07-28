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

#ifdef PBL_PLATFORM_BASALT
static void status_bar_background_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    GColor bg_color = curr_bg_color();
    GColor fg_color = curr_fg_color();
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_context_set_stroke_color(ctx, fg_color);
    
    // Fill background
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Show logo
    draw_image_in_rect(ctx, status_is_dark_theme()?RESOURCE_ID_IMG_LOGO_TRANSILIEN_DARK:RESOURCE_ID_IMG_LOGO_TRANSILIEN_LIGHT, GRect(4,3,69,10));
    
    // Separator
    graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w, bounds.size.h - 1));
}

static void status_bar_overlay_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    graphics_context_set_stroke_color(ctx, curr_fg_color());
    
    // Draw SNCF style time frame
    graphics_draw_round_rect(ctx,  bounds, 3);
}

void status_bar_set_colors(StatusBarLayer *status_bar_layer) {
    status_bar_layer_set_colors(status_bar_layer, status_is_dark_theme()?GColorBlack:GColorWhite, status_is_dark_theme()?GColorWhite:GColorBlack);
}

void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **status_bar_background_layer, Layer **status_bar_overlay_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    
    // Status bar background
    *status_bar_background_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_set_update_proc(*status_bar_background_layer, status_bar_background_layer_proc);
    layer_add_child(window_layer, *status_bar_background_layer);
    
    // Add status bar
    *status_bar_layer = status_bar_layer_create();
    GRect status_bar_frame = GRect(106, -1, 32, STATUS_BAR_LAYER_HEIGHT);
    layer_set_frame(status_bar_layer_get_layer(*status_bar_layer), status_bar_frame);
    layer_add_child(window_layer, status_bar_layer_get_layer(*status_bar_layer));
    // Setup status bar
    status_bar_set_colors(*status_bar_layer);
    
    // Add status bar overlay
    *status_bar_overlay_layer = layer_create(GRect(status_bar_frame.origin.x, status_bar_frame.origin.y - 3, status_bar_frame.size.w, status_bar_frame.size.h + 2));
    layer_set_update_proc(*status_bar_overlay_layer, status_bar_overlay_layer_proc);
    layer_add_child(window_layer, *status_bar_overlay_layer);
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
