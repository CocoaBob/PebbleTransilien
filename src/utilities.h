#pragma once

#define CELL_MARGIN 4
#define CELL_HEIGHT 32
#define CELL_ICON_SIZE 19
#define SEPARATOR_HEIGHT 1
#define TEXT_Y_OFFSET 2
#define HEADER_HEIGHT 15
  
#define THEME_BG_COLOR GColorWhite
#define THEME_FG_COLOR GColorBlack

// Drawing

void draw_text(GContext *ctx, const char * text, GColor color, const char * font_key, GRect frame, GTextAlignment alignment);

#ifdef PBL_SDK_3
void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **battery_layer);
#endif

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color);