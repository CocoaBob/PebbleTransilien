//
//  utilities.h
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define CELL_MARGIN 4
#define CELL_HEIGHT 44
#define CELL_HEIGHT_2 22
#define CELL_ICON_SIZE 19
#define SEPARATOR_HEIGHT 1
#define CELL_TEXT_Y_OFFSET 2
#define HEADER_HEIGHT 15

// Drawing

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment);

#ifdef PBL_SDK_3
void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **battery_layer);
#endif

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color);

// Settings
bool get_setting_theme();
void set_setting_theme(bool is_dark);
GColor curr_fg_color();
GColor curr_bg_color();