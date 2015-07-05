#include <pebble.h>
#include "main_menu_window.h"
#include "next_trains_window.h"
#include "utilities.h"

enum {
  MAIN_MENU_SECTION_FAV = 0,
  MAIN_MENU_SECTION_APP,
  MAIN_MENU_SECTION_COUNT
};

#define CELL_MARGIN 4
#define CELL_HEIGHT 32
#define CELL_ICON_SIZE 19
#define SEPARATOR_HEIGHT 1
#define TEXT_Y_OFFSET 2

#define HEADER_HEIGHT 15

Window *s_main_window;
static MenuLayer *s_menu_layer;
GColor s_bg_color;
GColor s_fg_color;

// Drawing

static void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title) {
  GRect bounds = layer_get_bounds(cell_layer);
  GRect frame = GRect(CELL_MARGIN,
                      -TEXT_Y_OFFSET,
                      bounds.size.w - CELL_MARGIN * 2,
                      bounds.size.h);
  
  draw_text(ctx, title, s_fg_color, FONT_KEY_GOTHIC_14, frame, GTextAlignmentCenter);
}

static void draw_from_to_cell(GContext *ctx, Layer *cell_layer, GColor stroke_color, const char * lineNum, const char * fromStation, const char * toStation) {
  GRect bounds = layer_get_bounds(cell_layer);
  int16_t semiHeight = bounds.size.h / 2;
  int16_t icon_size = (lineNum == NULL)?0:CELL_ICON_SIZE;
  GRect frame0 = GRect(CELL_MARGIN,
                       (CELL_HEIGHT - icon_size) / 2,
                       icon_size,
                       icon_size);
  GRect frame1 = GRect(CELL_MARGIN + icon_size + CELL_MARGIN,
                       -TEXT_Y_OFFSET,
                       bounds.size.w - (icon_size==0?0:CELL_MARGIN) - icon_size - CELL_MARGIN * 2,
                       semiHeight);
  GRect frame2 = GRect(CELL_MARGIN + icon_size + CELL_MARGIN,
                       semiHeight - TEXT_Y_OFFSET,
                       bounds.size.w - (icon_size==0?0:CELL_MARGIN) - icon_size - CELL_MARGIN * 2,
                       semiHeight);
  
  if (lineNum != NULL) {
    graphics_context_set_stroke_color(ctx, stroke_color);
    graphics_draw_round_rect(ctx, frame0, 3);
    draw_text(ctx, lineNum, stroke_color, FONT_KEY_GOTHIC_14_BOLD, frame0, GTextAlignmentCenter);
  }
  draw_text(ctx, fromStation, stroke_color, FONT_KEY_GOTHIC_14, frame1, GTextAlignmentLeft);
  draw_text(ctx, toStation, stroke_color, FONT_KEY_GOTHIC_14, frame2, GTextAlignmentLeft);
}

static void draw_single_line_cell(GContext *ctx, Layer *cell_layer, GColor stroke_color, const GBitmap * icon, const char * title, const char * font_key) {
  GRect bounds = layer_get_bounds(cell_layer);
  int16_t icon_size = (icon == NULL)?0:CELL_ICON_SIZE;
  GRect titleBox = GRect(CELL_MARGIN,
                         0,
                         bounds.size.w - (icon_size==0?0:CELL_MARGIN) - icon_size - CELL_MARGIN * 2,
                         bounds.size.h);
  GSize titleSize = graphics_text_layout_get_content_size(title,
                                                          fonts_get_system_font(font_key),
                                                          titleBox,
                                                          GTextOverflowModeTrailingEllipsis,
                                                          GTextAlignmentLeft);
  GRect titleFrame = GRect(CELL_MARGIN,
                      (bounds.size.h - titleSize.h) / 2 - TEXT_Y_OFFSET,
                      bounds.size.w - (icon_size==0?0:CELL_MARGIN) - icon_size - CELL_MARGIN * 2,
                      bounds.size.h);
  draw_text(ctx, title, stroke_color, font_key, titleFrame, GTextAlignmentLeft);
}

// Menu layer callbacks

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
  return MAIN_MENU_SECTION_COUNT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  switch(section_index) {
    case MAIN_MENU_SECTION_FAV:
    return 2;
    break;
    case MAIN_MENU_SECTION_APP:
    return 2;
    break;
    default:
    return 0;
    break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->section) {
    case MAIN_MENU_SECTION_FAV:
    {
      switch(cell_index->row) {
        case 0: return CELL_HEIGHT;
        case 1: return CELL_HEIGHT;
      }
    }
    case MAIN_MENU_SECTION_APP: return CELL_HEIGHT;
  }
  return 44;
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return HEADER_HEIGHT;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
  MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
  bool is_highlighted = (menu_index_compare(&selected_index, cell_index) == 0);
  GColor stroke_color = is_highlighted?GColorWhite:s_fg_color;
  switch(cell_index->section) {
    case MAIN_MENU_SECTION_FAV:
    {
      switch(cell_index->row) {
        case 0:
        draw_from_to_cell(ctx, cell_layer, stroke_color, "L", "Paris Saint-Lazare", "Bécon les Bruyères");
        break;
        case 1:
        draw_single_line_cell(ctx, cell_layer, stroke_color, NULL, "Bécon les Bruyères", FONT_KEY_GOTHIC_14);
        break;
        default:
        break;
      }
    }
    break;
    case MAIN_MENU_SECTION_APP:
    {
      switch(cell_index->row) {
        case 0:
        draw_single_line_cell(ctx, cell_layer, stroke_color, NULL, "Nearby Stations", FONT_KEY_GOTHIC_18_BOLD);
        break;
        case 1:
        draw_single_line_cell(ctx, cell_layer, stroke_color, NULL, "Search Name", FONT_KEY_GOTHIC_18_BOLD);
        break;
        default:
        break;
      }
    }
    break;
    default:
    break;
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  switch(section_index) {
    case MAIN_MENU_SECTION_FAV:
    draw_menu_header(ctx, cell_layer, "Favorites");
    break;
    case MAIN_MENU_SECTION_APP:
    draw_menu_header(ctx, cell_layer, "Search");
    break;
    default:
    break;
  }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->section) {
    case MAIN_MENU_SECTION_FAV:
    push_next_trains_window();
    break;
    case MAIN_MENU_SECTION_APP:
    push_next_trains_window();
    break;
    default:
    break;
  }
}

static int16_t get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  return SEPARATOR_HEIGHT;
}

static void draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)  {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_stroke_color(ctx, s_fg_color);
  for (int16_t dx = 0; dx < bounds.size.w; dx+=2) {
    graphics_draw_pixel(ctx, GPoint(bounds.origin.x + dx, bounds.origin.y));
  }
}

// Window load/unload

static void window_load(Window *window) {
  s_bg_color = GColorWhite;
  s_fg_color = GColorBlack;
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)get_num_sections_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
    .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
    .get_header_height = (MenuLayerGetHeaderHeightCallback)get_header_height_callback,
    .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
    .draw_header = (MenuLayerDrawHeaderCallback)draw_header_callback,
    .select_click = (MenuLayerSelectCallback)select_callback,
    .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback,
    .draw_separator = (MenuLayerDrawSeparatorCallback)draw_separator_callback,
  });
  menu_layer_set_normal_colors(s_menu_layer, s_bg_color, s_fg_color);
  menu_layer_set_highlight_colors(s_menu_layer, GColorCobaltBlue, GColorWhite);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_main_window = NULL;
}

// Entry point

void push_main_menu_window() {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  window_stack_push(s_main_window, true);
}