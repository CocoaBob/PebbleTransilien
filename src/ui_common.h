//
//  ui_common.h
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

#include <pebble.h>

// MARK: Darwing

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment);

#ifdef PBL_COLOR
void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);
#endif

#if !defined(PBL_PLATFORM_APLITE)
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color);
#endif

void draw_image_in_rect(GContext* ctx,
#ifdef PBL_BW
                        bool is_inverted,
#endif
                        uint32_t resource_id,
                        GRect rect);

void draw_centered_title(GContext* ctx,
                         const Layer *cell_layer,
#ifdef PBL_BW
                         bool is_inverted,
#endif
                         const char *title,
                         const char *font_id);

void draw_from_to(GContext* ctx,
                  const Layer *layer,
#ifdef PBL_COLOR
                  bool is_highlighed,
                  GColor text_color,
#else
                  bool is_inverted,
#endif
                  DataModelFromTo from_to);

void draw_station(GContext *ctx, Layer *cell_layer,
#ifdef PBL_COLOR
                  GColor text_color,
                  bool is_highlighed,
#else
                  bool is_inverted,
#endif
                  const char * str_time,
                  const char * str_station);

// MARK: Menu Layer Callbacks

void menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context);
void menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context);


#if !defined(PBL_PLATFORM_APLITE)
int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
void menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context);
#endif