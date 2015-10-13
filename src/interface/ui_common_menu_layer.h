//
//  ui_common_menu_layer.h
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

#include <pebble.h>

// MARK: Menu Layer

void menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context);
void menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context);


#if !defined(PBL_PLATFORM_APLITE)
int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
void menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context);
#endif

// MARK: Action list callbacks

#ifdef PBL_COLOR
GColor action_list_get_bar_color(void);
#endif