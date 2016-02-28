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

void ui_setup_status_bars(Layer *window_layer, Layer *sibling_layer) {
    GRect bounds = layer_get_bounds(window_layer);
    
    Layer *status_bar_layer = status_bar(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_insert_below_sibling(status_bar_layer, sibling_layer);
#ifdef PBL_ROUND
    Layer *round_bottom_bar_layer = round_bottom_bar(GRect(bounds.origin.x, bounds.origin.y + bounds.size.h - STATUS_BAR_LAYER_HEIGHT, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_insert_below_sibling(round_bottom_bar_layer, sibling_layer);
#endif
}

// MARK: Theme

void ui_setup_theme(MenuLayer *menu_layer) {
    menu_layer_set_normal_colors(menu_layer, curr_bg_color(), curr_fg_color());
    menu_layer_set_highlight_colors(menu_layer, PBL_IF_COLOR_ELSE(HIGHLIGHT_COLOR, curr_fg_color()), PBL_IF_COLOR_ELSE(GColorWhite, curr_bg_color()));
}

#ifdef PBL_ROUND
// MARK: Round bottom

static Layer *s_round_bottom_bar_layer;

static void round_bottom_bar_background_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    // Background
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Separator
    graphics_context_set_stroke_color(ctx, curr_fg_color());
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));
}

// Get round bottom bar layer
Layer *round_bottom_bar(GRect frame) {
    layer_set_frame(s_round_bottom_bar_layer, frame);
    return s_round_bottom_bar_layer;
}

// Update status bar drawing
void round_bottom_bar_update() {
    layer_mark_dirty(s_round_bottom_bar_layer);
}

// MARK: Setup

void round_bottom_bar_init() {
    s_round_bottom_bar_layer = layer_create_with_data(GRect(0, 0, 144, STATUS_BAR_LAYER_HEIGHT), 0);
    layer_set_update_proc(s_round_bottom_bar_layer, round_bottom_bar_background_layer_proc);
}

void round_bottom_bar_deinit() {
    layer_destroy(s_round_bottom_bar_layer);
}

#endif