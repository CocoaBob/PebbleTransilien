//
//  ui_common_menu_layer.c
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Menu Layer

void menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex old_index = menu_layer_get_selected_index(context);
    if (old_index.section == 0 && old_index.row == 0) {
        menu_layer_set_selected_index(context, MenuIndex(UINT16_MAX, UINT16_MAX), MenuRowAlignBottom, true);
    } else {
        menu_layer_set_selected_next(context, true, MenuRowAlignCenter, true);
    }
}

void menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex old_index = menu_layer_get_selected_index(context);
    menu_layer_set_selected_next(context, false, MenuRowAlignCenter, true);
    MenuIndex new_index = menu_layer_get_selected_index(context);
    if (menu_index_compare(&old_index, &new_index) == 0) {
        menu_layer_set_selected_index(context, MenuIndex(0, 0), MenuRowAlignTop, true);
    }
}

#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)

int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 1;
}

void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

void menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context) {
    GRect frame = layer_get_frame(bg_layer);
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
}
#endif

// MARK: Action list callbacks

#ifdef PBL_COLOR
GColor action_list_get_bar_color(void) {
    return GColorCobaltBlue;
}
#endif