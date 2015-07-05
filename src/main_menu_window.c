#include <pebble.h>
#include "main_menu_window.h"
#include "next_trains_window.h"
#include "utilities.h"

enum {
  MAIN_MENU_SECTION_FAV = 0,
  MAIN_MENU_SECTION_APP,
  MAIN_MENU_SECTION_COUNT
};

Window *s_main_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_SDK_3
static StatusBarLayer *s_status_bar;
static Layer *s_battery_layer;
#endif

// Drawing

void draw_main_menu_cell(GContext *ctx, Layer *cell_layer, GColor stroke_color,
                    const char * str_icon, const char * icon_font_key,
                    const char * str_line_1, const char * line_1_font_key,
                    const char * str_line_2, const char * line_2_font_key) {
  GRect bounds = layer_get_bounds(cell_layer);

  GRect frame_icon = GRect(CELL_MARGIN,
                           (CELL_HEIGHT - CELL_ICON_SIZE) / 2,
                           CELL_ICON_SIZE,
                           CELL_ICON_SIZE);

  graphics_context_set_stroke_color(ctx, stroke_color);
  graphics_draw_round_rect(ctx, frame_icon, 3);
  draw_text(ctx, str_icon, stroke_color, icon_font_key, frame_icon, GTextAlignmentCenter);

  if (str_line_2 != NULL) {
    int16_t semi_height = bounds.size.h / 2;
    
    GRect frame_line_1 = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                             -TEXT_Y_OFFSET,
                             bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN * 3,
                             semi_height);
    draw_text(ctx, str_line_1, stroke_color, line_1_font_key, frame_line_1, GTextAlignmentLeft);

    GRect frame_line_2 = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                             semi_height - TEXT_Y_OFFSET,
                             bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN * 3,
                             semi_height);
    draw_text(ctx, str_line_2, stroke_color, line_2_font_key, frame_line_2, GTextAlignmentLeft);
  } else {
    GRect frame_line_1 = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                             5-TEXT_Y_OFFSET,
                             bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN * 3,
                             bounds.size.h - 4);
    draw_text(ctx, str_line_1, stroke_color, line_1_font_key, frame_line_1, GTextAlignmentLeft);
  }
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
  if (cell_index->section == MAIN_MENU_SECTION_FAV ) {
    switch(cell_index->row) {
      case 0:
      draw_main_menu_cell(ctx, cell_layer, stroke_color,
                          "L", FONT_KEY_GOTHIC_14_BOLD,
                          "Paris Saint-Lazare", FONT_KEY_GOTHIC_14,
                          "Bécon les Bruyères", FONT_KEY_GOTHIC_14);
      break;
      case 1:
      draw_main_menu_cell(ctx, cell_layer, stroke_color,
                          "All", FONT_KEY_GOTHIC_14,
                          "Paris Saint-Lazare", FONT_KEY_GOTHIC_18,
                          NULL, NULL);
      break;
      default:
      break;
    }
  }
  else if (cell_index->section == MAIN_MENU_SECTION_APP) {
    switch(cell_index->row) {
      case 0:
      draw_main_menu_cell(ctx, cell_layer, stroke_color,
                          "?", FONT_KEY_GOTHIC_14,
                          "Nearby stations", FONT_KEY_GOTHIC_18,
                          NULL, NULL);
      break;
      case 1:
      draw_main_menu_cell(ctx, cell_layer, stroke_color,
                          "a_", FONT_KEY_GOTHIC_14,
                          "Search by name", FONT_KEY_GOTHIC_18,
                          NULL, NULL);
      break;
      default:
      break;
    }
  }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  switch(section_index) {
    case MAIN_MENU_SECTION_FAV:
    draw_menu_header(ctx, cell_layer, "Favorites", THEME_FG_COLOR);
    break;
    case MAIN_MENU_SECTION_APP:
    draw_menu_header(ctx, cell_layer, "Search", THEME_FG_COLOR);
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
#ifdef PBL_SDK_3
  layer_destroy(s_battery_layer);
  status_bar_layer_destroy(s_status_bar);
#endif
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