#include <pebble.h>
#include "next_trains_window.h"
#include "utilities.h"

enum {
  NEXT_TRAINS_SECTION_INVERSE = 0,
  NEXT_TRAINS_SECTION_LIST,
  NEXT_TRAINS_SECTION_COUNT
};

static Window *s_main_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_SDK_3
static StatusBarLayer *s_status_bar;
static Layer *s_battery_layer;
#endif

// Drawing

void draw_next_trains_cell(GContext *ctx, Layer *cell_layer, GColor stroke_color,
                    const char * str_line, const char * font_key_line,
                    const char * str_code, const char * font_key_code,
                    const char * str_time, const char * font_key_time,
                    const char * str_terminus, const char * font_key_terminus,
                    const char * str_platform, const char * font_key_platform) {
  GRect bounds = layer_get_bounds(cell_layer);

  // Line
  GRect frame_line = GRect(CELL_MARGIN,
                           (CELL_HEIGHT - CELL_ICON_SIZE) / 2,
                           CELL_ICON_SIZE,
                           CELL_ICON_SIZE);
  graphics_context_set_stroke_color(ctx, stroke_color);
  graphics_draw_round_rect(ctx, frame_line, 3);
  draw_text(ctx, str_line, stroke_color, font_key_line, frame_line, GTextAlignmentCenter);

  int16_t semi_height = bounds.size.h / 2;
  int16_t semi_content_width = bounds.size.w / 2 - CELL_ICON_SIZE - CELL_MARGIN * 2;
  
  // Code
  GRect frame_code = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                           -TEXT_Y_OFFSET,
                           semi_content_width,
                           semi_height);
  draw_text(ctx, str_code, stroke_color, font_key_code, frame_code, GTextAlignmentLeft);

  // Time
  GRect frame_time = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN + semi_content_width,
                           - TEXT_Y_OFFSET,
                           semi_content_width,
                           semi_height);
  draw_text(ctx, str_time, stroke_color, font_key_time, frame_time, GTextAlignmentRight);

  // Terminus
  GRect frame_terminus = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                               semi_height - TEXT_Y_OFFSET,
                               bounds.size.w - CELL_ICON_SIZE * 2 - CELL_MARGIN * 4,
                               semi_height);
  draw_text(ctx, str_terminus, stroke_color, font_key_terminus, frame_terminus, GTextAlignmentLeft);

  // Platform
  GRect frame_icon_2 = GRect(bounds.size.w - CELL_MARGIN - CELL_ICON_SIZE,
                             (CELL_HEIGHT - CELL_ICON_SIZE) / 2,
                             CELL_ICON_SIZE,
                             CELL_ICON_SIZE);

  graphics_context_set_stroke_color(ctx, stroke_color);
  graphics_draw_round_rect(ctx, frame_icon_2, 3);
  draw_text(ctx, str_platform, stroke_color, font_key_platform, frame_icon_2, GTextAlignmentCenter);
}

// Menu layer callbacks

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
  return NEXT_TRAINS_SECTION_COUNT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  switch(section_index) {
    case NEXT_TRAINS_SECTION_INVERSE:
    return 1;
    break;
    case NEXT_TRAINS_SECTION_LIST:
    return 5;
    break;
    default:
    return 0;
    break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return CELL_HEIGHT;
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return HEADER_HEIGHT;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
#ifdef PBL_COLOR
  MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
  bool is_highlighted = (menu_index_compare(&selected_index, cell_index) == 0);
  GColor stroke_color = is_highlighted?GColorWhite:THEME_FG_COLOR;
#else
  GColor stroke_color = THEME_FG_COLOR;
#endif
  if (cell_index->section == NEXT_TRAINS_SECTION_INVERSE ) {

  }
  else if (cell_index->section == NEXT_TRAINS_SECTION_LIST ) {
    switch(cell_index->row) {
      case 0:
      draw_next_trains_cell(ctx, cell_layer, stroke_color,
                            "J", FONT_KEY_GOTHIC_14_BOLD,
                            "EAPE", FONT_KEY_GOTHIC_14,
                            "00:01", FONT_KEY_GOTHIC_14,
                            "Ermont Eaubonne", FONT_KEY_GOTHIC_14,
                            "9", FONT_KEY_GOTHIC_14_BOLD);
      break;
      case 1:
      draw_next_trains_cell(ctx, cell_layer, stroke_color,
                            "L", FONT_KEY_GOTHIC_14_BOLD,
                            "NOPE", FONT_KEY_GOTHIC_14,
                            "00:04", FONT_KEY_GOTHIC_14,
                            "Nanterre Université", FONT_KEY_GOTHIC_14,
                            "?", FONT_KEY_GOTHIC_14_BOLD);
      break;
      case 2:
      draw_next_trains_cell(ctx, cell_layer, stroke_color,
                            "J", FONT_KEY_GOTHIC_14_BOLD,
                            "TOCA", FONT_KEY_GOTHIC_14,
                            "00:10", FONT_KEY_GOTHIC_14,
                            "Pontoise", FONT_KEY_GOTHIC_14,
                            "?", FONT_KEY_GOTHIC_14_BOLD);
      break;
      case 3:
      draw_next_trains_cell(ctx, cell_layer, stroke_color,
                            "L", FONT_KEY_GOTHIC_14_BOLD,
                            "SEBO", FONT_KEY_GOTHIC_14,
                            "00:13", FONT_KEY_GOTHIC_14,
                            "Saint-Nom-la-Bretèche - Forêt de Marly", FONT_KEY_GOTHIC_14,
                            "?", FONT_KEY_GOTHIC_14_BOLD);
      break;
      case 4:
      draw_next_trains_cell(ctx, cell_layer, stroke_color,
                            "L", FONT_KEY_GOTHIC_14_BOLD,
                            "FOPE", FONT_KEY_GOTHIC_14,
                            "00:19", FONT_KEY_GOTHIC_14,
                            "Maisons Laffitte", FONT_KEY_GOTHIC_14,
                            "?", FONT_KEY_GOTHIC_14_BOLD);
      break;
      default:
      break;
    }
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  switch(section_index) {
    case NEXT_TRAINS_SECTION_INVERSE:
    draw_menu_header(ctx, cell_layer, "Inverse directions", THEME_FG_COLOR);
    break;
    case NEXT_TRAINS_SECTION_LIST:
    draw_menu_header(ctx, cell_layer, "Next trains", THEME_FG_COLOR);
    break;
    default:
    break;
  }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->section) {
    case NEXT_TRAINS_SECTION_INVERSE:

    break;
    case NEXT_TRAINS_SECTION_LIST:
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
  draw_separator(ctx, cell_layer, THEME_FG_COLOR);
}

// Window load/unload

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Add menu layer
  int16_t status_bar_height = 0;
#ifdef PBL_SDK_3
  status_bar_height = STATUS_BAR_LAYER_HEIGHT;
#endif
  GRect menu_layer_frame = GRect(bounds.origin.x,
                                 bounds.origin.y + status_bar_height,
                                 bounds.size.w,
                                 bounds.size.h - status_bar_height);
  s_menu_layer = menu_layer_create(menu_layer_frame);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  // Setup menu layer
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
#ifdef PBL_COLOR
  menu_layer_set_normal_colors(s_menu_layer, THEME_BG_COLOR, THEME_FG_COLOR);
  menu_layer_set_highlight_colors(s_menu_layer, GColorCobaltBlue, GColorWhite);
#endif
  
  // Finally, add status bar
#ifdef PBL_SDK_3
  window_add_status_bar(window_layer, &s_status_bar, &s_battery_layer);
#endif
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
  s_main_window = NULL;
}

// Entry point

void push_next_trains_window() {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
  }
  window_stack_push(s_main_window, true);
}
