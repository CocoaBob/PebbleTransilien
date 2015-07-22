//
//  next_trains_window.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

enum {
    NEXT_TRAINS_SECTION_INFO = 0,
    NEXT_TRAINS_SECTION_TRAINS,
    NEXT_TRAINS_SECTION_COUNT
};

static Window *s_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_PLATFORM_BASALT
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_overlay_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

static DataModelFromTo *s_from_to;
static char *s_str_from;
static char *s_str_to;

static size_t s_next_trains_count;
static DataModelNextTrain *s_next_trains;

// MARK: Constants

#define NEXT_TRAIN_CELL_ICON_SIZE 19
#define NEXT_TRAIN_CELL_ICON_W 27  // CELL_MARGIN + NEXT_TRAIN_CELL_ICON_SIZE + CELL_MARGIN = 4 + 19 + 4
#define NEXT_TRAIN_CELL_ICON_Y 4   // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_X 27  // CELL_MARGIN + NEXT_TRAIN_CELL_ICON_SIZE + CELL_MARGIN = 4 + 19 + 4
#define NEXT_TRAIN_CELL_CODE_W 46
#define NEXT_TRAIN_CELL_TIME_X 73  // CELL_MARGIN + NEXT_TRAIN_CELL_ICON_SIZE + CELL_MARGIN + NEXT_TRAIN_CELL_CODE_W = 4 + 19 + 4 + 48

// MARK: Drawing

void draw_next_trains_info(GContext *ctx,
                           Layer *cell_layer,
                           GColor stroke_color
#ifdef PBL_COLOR
                           ,bool is_highlighed
#endif
                           )
{
    graphics_context_set_text_color(ctx, stroke_color);
    graphics_context_set_stroke_color(ctx, stroke_color);
    GRect bounds = layer_get_bounds(cell_layer);
    bool is_from_to = (s_from_to->to != STATION_NON);
    
    // Draw left icon
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT - FROM_TO_ICON_HEIGHT) / 2,
                             FROM_TO_ICON_WIDTH,
                             is_from_to?FROM_TO_ICON_HEIGHT:FROM_TO_ICON_WIDTH);
#ifdef PBL_COLOR
    if (is_from_to) {
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_TO_DARK:RESOURCE_ID_IMG_FROM_TO_LIGHT, frame_icon);
    } else {
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    }
#else
    if (is_from_to) {
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_FROM_TO_LIGHT, frame_icon);
    } else {
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    }
#endif
    
    // Draw lines
    GRect frame_line_1 = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                               -CELL_TEXT_Y_OFFSET + 2, // +2 to get the two lines closer
                               bounds.size.w - FROM_TO_ICON_WIDTH - CELL_MARGIN * 3,
                               CELL_HEIGHT_2);
    if (is_from_to) {
        draw_text(ctx, s_str_from, FONT_KEY_GOTHIC_18_BOLD, frame_line_1, GTextAlignmentLeft);
        
        GRect frame_line_2 = frame_line_1;
        frame_line_2.origin.y = CELL_HEIGHT_2 - CELL_TEXT_Y_OFFSET - 2; // -2 to get the two lines closer
        
        draw_text(ctx, s_str_to, FONT_KEY_GOTHIC_18_BOLD, frame_line_2, GTextAlignmentLeft);
    } else {
        frame_line_1.size.h = bounds.size.h;
        GSize text_size = graphics_text_layout_get_content_size(s_str_from,
                                                                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                                                                frame_line_1,
                                                                GTextOverflowModeTrailingEllipsis,
                                                                GTextAlignmentLeft);
        frame_line_1.size.h = text_size.h;
        
        draw_text(ctx, s_str_from, FONT_KEY_GOTHIC_18_BOLD, frame_line_1, GTextAlignmentLeft);
    }
}

void draw_next_trains_cell(GContext *ctx, Layer *cell_layer,
                           GColor stroke_color,
#ifdef PBL_COLOR
                           bool is_highlighed,
#endif
                           const char * str_line,
                           const char * str_code,
                           const char * str_time,
                           const char * str_terminus,
                           const char * str_platform) {
    graphics_context_set_text_color(ctx, stroke_color);
    graphics_context_set_stroke_color(ctx, stroke_color);
    GRect bounds = layer_get_bounds(cell_layer);
    
    // Line
    GRect frame_line = GRect(CELL_MARGIN,
                             NEXT_TRAIN_CELL_ICON_Y,
                             NEXT_TRAIN_CELL_ICON_SIZE,
                             NEXT_TRAIN_CELL_ICON_SIZE);
#ifdef PBL_COLOR
    draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FRAME_DARK:RESOURCE_ID_IMG_FRAME_LIGHT, frame_line);
#else
    draw_image_in_rect(ctx, RESOURCE_ID_IMG_FRAME_LIGHT, frame_line);
#endif
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
                             bounds.size.w - NEXT_TRAIN_CELL_TIME_X - CELL_MARGIN - NEXT_TRAIN_CELL_ICON_SIZE - 4,
                             CELL_HEIGHT_2);
    draw_text(ctx, str_time, FONT_KEY_GOTHIC_24_BOLD, frame_time, GTextAlignmentLeft);
    
    // Terminus
    GRect frame_terminus = GRect(CELL_MARGIN,
                                 CELL_HEIGHT_2 - CELL_TEXT_Y_OFFSET + 1,
                                 bounds.size.w - CELL_MARGIN * 2,
                                 CELL_HEIGHT_2);
    draw_text(ctx, str_terminus, FONT_KEY_GOTHIC_18, frame_terminus, GTextAlignmentLeft);
    
    // Platform
    GRect frame_platform = GRect(bounds.size.w - NEXT_TRAIN_CELL_ICON_SIZE - CELL_MARGIN,
                               NEXT_TRAIN_CELL_ICON_Y,
                               NEXT_TRAIN_CELL_ICON_SIZE,
                               NEXT_TRAIN_CELL_ICON_SIZE);
    if (str_platform != NULL) {
#ifdef PBL_COLOR
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FRAME_DARK:RESOURCE_ID_IMG_FRAME_LIGHT, frame_platform);
#else
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_FRAME_LIGHT, frame_platform);
#endif
        draw_text(ctx, str_platform, FONT_KEY_GOTHIC_14_BOLD, frame_platform, GTextAlignmentCenter);
    } else {
#ifdef PBL_COLOR
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_DOTTED_FRAME_DARK:RESOURCE_ID_IMG_DOTTED_FRAME_LIGHT, frame_platform);
#else
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_DOTTED_FRAME_LIGHT, frame_platform);
#endif
    }
}

// MARK: Menu layer callbacks

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return NEXT_TRAINS_SECTION_COUNT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == NEXT_TRAINS_SECTION_INFO) {
        return 1;
    } else if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        return 5;
    }
    return 0;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT;
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        return HEADER_HEIGHT;
    }
    return 0;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
#ifdef PBL_COLOR
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (menu_index_compare(&selected_index, cell_index) == 0);
    bool is_highlighed = status_is_dark_theme() || is_selected;
    GColor stroke_color = is_selected?curr_bg_color():curr_fg_color();
#else
    GColor stroke_color = curr_fg_color();
#endif
    
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        draw_next_trains_info(ctx,
                              cell_layer,
                              stroke_color
#ifdef PBL_COLOR
                              ,is_highlighed
#endif
                              );
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        switch(cell_index->row) {
            case 0:
                draw_next_trains_cell(ctx, cell_layer,
                                      stroke_color,
#ifdef PBL_COLOR
                                      is_highlighed,
#endif
                                      "J",
                                      "EAPE",
                                      "00:01",
                                      "Ermont Eaubonne",
                                      "9");
                break;
            case 1:
                draw_next_trains_cell(ctx, cell_layer,
                                      stroke_color,
#ifdef PBL_COLOR
                                      is_highlighed,
#endif
                                      "L",
                                      "NOPE",
                                      "00:04",
                                      "Nanterre Université",
                                      "27");
                break;
            case 2:
                draw_next_trains_cell(ctx, cell_layer,
                                      stroke_color,
#ifdef PBL_COLOR
                                      is_highlighed,
#endif
                                      "J",
                                      "TOCA",
                                      "00:10",
                                      "Pontoise",
                                      "C");
                break;
            case 3:
                draw_next_trains_cell(ctx, cell_layer,
                                      stroke_color,
#ifdef PBL_COLOR
                                      is_highlighed,
#endif
                                      "L",
                                      "SEBO",
                                      "00:13",
                                      "Saint-Nom-la-Bretèche - Forêt de Marly",
                                      "A");
                break;
            case 4:
                draw_next_trains_cell(ctx, cell_layer,
                                      stroke_color,
#ifdef PBL_COLOR
                                      is_highlighed,
#endif
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
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
    if (section_index == NEXT_TRAINS_SECTION_INFO) {
        return;
    } else if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        draw_menu_header(ctx, cell_layer, _("Next trains"), curr_fg_color());
    }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        // TODO: Reverse direction
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        // TODO: Show train details
    }
}

static void select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    // TODO: Favorites
}

static int16_t get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return SEPARATOR_HEIGHT;
}

static void draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)  {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

#ifdef PBL_PLATFORM_BASALT
static void draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context) {
    GRect frame = layer_get_frame(bg_layer);
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
}
#endif

// MARK: Window callbacks

static void window_load(Window *window) {
    // Data
    s_str_from = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    stations_get_name(s_from_to->from, s_str_from, STATION_NAME_MAX_LENGTH);
    s_str_to = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    stations_get_name(s_from_to->to, s_str_to, STATION_NAME_MAX_LENGTH);
    
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    int16_t status_bar_height = 0;
#ifdef PBL_PLATFORM_BASALT
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
        .select_long_click = (MenuLayerSelectCallback)select_long_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)draw_separator_callback
#ifdef PBL_PLATFORM_BASALT
        ,
        .draw_background = (MenuLayerDrawBackgroundCallback)draw_background_callback
#endif
    });
    
    // Finally, add status bar
#ifdef PBL_PLATFORM_BASALT
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_overlay_layer);
#endif
    
#ifdef PBL_BW
    s_inverter_layer = inverter_layer_create(menu_layer_frame);
#endif
    
#ifdef PBL_COLOR
    setup_ui_theme(s_menu_layer);
#else
    setup_ui_theme(s_window, s_inverter_layer);
#endif
}

static void window_unload(Window *window) {
    // Data
    free(s_str_from);
    free(s_str_to);
    free(s_from_to);
    s_from_to = NULL;
    
    // Window
    menu_layer_destroy(s_menu_layer);
    window_destroy(window);
    s_window = NULL;
    
#ifdef PBL_PLATFORM_BASALT
    layer_destroy(s_status_bar_overlay_layer);
    status_bar_layer_destroy(s_status_bar);
#endif
    
#ifdef PBL_BW
    inverter_layer_destroy(s_inverter_layer);
#endif
}

static void window_appear(Window *window) {
    
}

static void window_disappear(Window *window) {
    
}

// MARK: Entry point

void push_next_trains_window(DataModelFromTo from_to) {
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
    }
    
    if (s_from_to != NULL) {
        free(s_from_to);
        s_from_to = NULL;
    }
    s_from_to = malloc(sizeof(DataModelFromTo));
    s_from_to->from = from_to.from;
    s_from_to->to = from_to.to;
    
    window_stack_push(s_window, true);
}
