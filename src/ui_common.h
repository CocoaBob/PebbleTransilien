//
//  ui_common.h
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

// MARK: Routines

GSize size_of_text(const char *text, const char *font_key, GRect frame);

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
                         bool is_inverted,
                         const char *title,
                         const char *font_id);

void draw_from_to(GContext* ctx, Layer *drawing_layer,
                  Layer *redraw_layer, bool is_selected,
#ifdef PBL_COLOR
                  bool is_highlighed,
                  GColor text_color,
#else
                  bool is_inverted,
#endif
                  DataModelFromTo from_to);

void draw_station(GContext *ctx, Layer *drawing_layer,
                  Layer *redraw_layer, bool is_selected,
#ifdef PBL_COLOR
                  GColor text_color,
                  bool is_highlighed,
#else
                  bool is_inverted,
#endif
                  char * str_time,
                  char * str_station);

// MARK: Menu Layer Callbacks

void menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context);
void menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context);

#if !defined(PBL_PLATFORM_APLITE)
int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
void menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context);
#endif

// MARK: Scroll Texts

void text_scroll_begin(Layer *menu_layer, const char* text, size_t const text_length, const char * font_key, const GRect text_frame);
void text_scroll_end();
bool text_scroll_is_on();
char *text_scroll_text(char* text, const char * font_key, const GRect text_frame, bool jump_accent);