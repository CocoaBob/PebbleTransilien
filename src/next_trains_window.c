#include <pebble.h>
#include "next_trains_window.h"
#include "utilities.h"

static Window *s_main_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_SDK_3
static StatusBarLayer *s_status_bar;
static Layer *s_battery_layer;
#endif

// Drawing
#define NEXT_TRAIN_CELL_ICON_W 27  // CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN = 4 + 19 + 4
#define NEXT_TRAIN_CELL_ICON_Y 4   // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_X 27  // CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN = 4 + 19 + 4
#define NEXT_TRAIN_CELL_CODE_W 46
#define NEXT_TRAIN_CELL_TIME_X 73  // CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN + NEXT_TRAIN_CELL_CODE_W = 4 + 19 + 4 + 48

void draw_next_trains_cell(GContext *ctx, Layer *cell_layer, GColor stroke_color,
                           const char * str_line,
                           const char * str_code,
                           const char * str_time,
                           const char * str_terminus,
                           const char * str_platform) {
  graphics_context_set_stroke_color(ctx, stroke_color);
  GRect bounds = layer_get_bounds(cell_layer);

  // Line
  GRect frame_line = GRect(CELL_MARGIN,
                           NEXT_TRAIN_CELL_ICON_Y,
                           CELL_ICON_SIZE,
                           CELL_ICON_SIZE);
  graphics_draw_round_rect(ctx, frame_line, 3);
  draw_text(ctx, str_line, FONT_KEY_GOTHIC_14_BOLD, frame_line, GTextAlignmentCenter);
  
  // Code
  GRect frame_code = GRect(NEXT_TRAIN_CELL_CODE_X,
                           -CELL_TEXT_Y_OFFSET - 2,
                           NEXT_TRAIN_CELL_CODE_W,
                           CELL_HEIGHT_2);
  draw_text(ctx, str_code, FONT_KEY_GOTHIC_24_BOLD, frame_code, GTextAlignmentLeft);

  // Time
  GRect frame_time = GRect(NEXT_TRAIN_CELL_TIME_X,
                           - CELL_TEXT_Y_OFFSET - 2,
                           bounds.size.w - NEXT_TRAIN_CELL_TIME_X - CELL_MARGIN - CELL_ICON_SIZE - 4,
                           CELL_HEIGHT_2);
  draw_text(ctx, str_time, FONT_KEY_GOTHIC_24_BOLD, frame_time, GTextAlignmentLeft);

  // Terminus
  GRect frame_terminus = GRect(CELL_MARGIN,
                               CELL_HEIGHT_2 - CELL_TEXT_Y_OFFSET + 1,
                               bounds.size.w - CELL_MARGIN * 2,
                               CELL_HEIGHT_2);
  draw_text(ctx, str_terminus, FONT_KEY_GOTHIC_18, frame_terminus, GTextAlignmentLeft);

  // Platform
  bool has_platform = str_platform != NULL;
  if (has_platform) {
    GRect frame_icon_2 = GRect(bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN,
                               NEXT_TRAIN_CELL_ICON_Y,
                               CELL_ICON_SIZE,
                               CELL_ICON_SIZE);
    graphics_draw_round_rect(ctx, frame_icon_2, 3);
    draw_text(ctx, str_platform, FONT_KEY_GOTHIC_14, frame_icon_2, GTextAlignmentCenter);
  }
}

// Menu layer callbacks

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 5;
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
  switch(cell_index->row) {
  case 0:
  draw_next_trains_cell(ctx, cell_layer, stroke_color,
                        "J",
                        "EAPE",
                        "00:01",
                        "Ermont Eaubonne",
                        "9");
  break;
  case 1:
  draw_next_trains_cell(ctx, cell_layer, stroke_color,
                        "L",
                        "NOPE",
                        "00:04",
                        "Nanterre Université",
                        "27");
  break;
  case 2:
  draw_next_trains_cell(ctx, cell_layer, stroke_color,
                        "J",
                        "TOCA",
                        "00:10",
                        "Pontoise",
                        "C");
  break;
  case 3:
  draw_next_trains_cell(ctx, cell_layer, stroke_color,
                        "L",
                        "SEBO",
                        "00:13",
                        "Saint-Nom-la-Bretèche - Forêt de Marly",
                        "A");
  break;
  case 4:
  draw_next_trains_cell(ctx, cell_layer, stroke_color,
                        "L",
                        "FOPE",
                        "00:19",
                        "Maisons Laffitte",
                        NULL);
  break;
  default:
  break;
}
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  draw_menu_header(ctx, cell_layer, "Next trains", THEME_FG_COLOR);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  push_next_trains_window();
}

static void select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  push_next_trains_window();
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
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
    .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
    .get_header_height = (MenuLayerGetHeaderHeightCallback)get_header_height_callback,
    .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
    .draw_header = (MenuLayerDrawHeaderCallback)draw_header_callback,
    .select_click = (MenuLayerSelectCallback)select_callback,
    .select_long_click = (MenuLayerSelectCallback)select_long_callback,
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
