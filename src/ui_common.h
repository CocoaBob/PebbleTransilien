//
//  ui_common.h
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

// MARK: Routines

#ifdef PBL_ROUND
void ui_common_init();
void ui_common_deinit();
#endif

GSize size_of_text(const char *text, const char *font_key, GRect frame);

// MARK: Darwing

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment);

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);

void draw_image_in_rect(GContext* ctx,
                        uint32_t resource_id,
                        GRect rect);

void draw_centered_title(GContext* ctx,
                         const Layer *cell_layer,
                         bool is_inverted,
                         const char *title,
                         const char *font_id);

void draw_from_to(GContext* ctx, Layer *drawing_layer,
#if TEXT_SCROLL_IS_ENABLED
                  TextScrollData **ptr_text_scroll_data, Layer *redraw_layer, bool is_selected,
#endif
                  bool is_inverted,
                  GColor fg_color,
                  DataModelFromTo from_to
#if MINI_TIMETABLE_IS_ENABLED
                  ,
                  bool draw_mini_timetable
#endif
#if EXTRA_INFO_IS_ENABLED
                  ,
                  bool draw_extra_info_indicator
#endif
                  );

void draw_station(GContext *ctx, Layer *drawing_layer,
#if TEXT_SCROLL_IS_ENABLED
                  TextScrollData **ptr_text_scroll_data, Layer *redraw_layer, bool is_selected,
#endif
                  GColor text_color,
                  bool is_inverted,
                  char * str_time,
                  char * str_station);

// MARK: Menu Layer Click Config Provider handlers
void common_menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context);
void common_menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context);

// MARK: Menu Layer Callbacks
int16_t common_menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
void common_menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
void common_menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context);

// MARK: Push windows
void ui_push_window(Window *window);