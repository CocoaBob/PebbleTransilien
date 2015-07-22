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
static bool s_is_updating;

// MARK: Constants

#define NEXT_TRAIN_CELL_ICON_SIZE 19
#define NEXT_TRAIN_CELL_ICON_W 27   // CELL_MARGIN + NEXT_TRAIN_CELL_ICON_SIZE + CELL_MARGIN = 4 + 19 + 4
#define NEXT_TRAIN_CELL_ICON_Y 4    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_X 4    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_W 56
#define NEXT_TRAIN_CELL_TIME_X 64   // CELL_MARGIN + NEXT_TRAIN_CELL_CODE_W + CELL_MARGIN = 4 + 56

// MARK: Drawing

void draw_next_trains_info(GContext *ctx,
                           Layer *cell_layer,
                           GColor text_color
#ifdef PBL_COLOR
                           ,bool is_highlighed
#endif
                           )
{
    graphics_context_set_text_color(ctx, text_color);
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
                           GColor text_color,
#ifdef PBL_COLOR
                           bool is_highlighed,
#endif
                           const char * str_code,
                           const char * str_time,
                           const char * str_terminus,
                           const char * str_platform) {
    graphics_context_set_text_color(ctx, text_color);
    GRect bounds = layer_get_bounds(cell_layer);
    
    // Code
    GRect frame_code = GRect(NEXT_TRAIN_CELL_CODE_X,
                             -CELL_TEXT_Y_OFFSET - 2,
                             NEXT_TRAIN_CELL_CODE_W,
                             CELL_HEIGHT_2);
    draw_text(ctx, str_code, FONT_KEY_GOTHIC_24_BOLD, frame_code, GTextAlignmentLeft);
    
    // Time
    GRect frame_time = GRect(NEXT_TRAIN_CELL_TIME_X,
                             - CELL_TEXT_Y_OFFSET - 2,
                             bounds.size.w - NEXT_TRAIN_CELL_TIME_X - CELL_MARGIN - NEXT_TRAIN_CELL_ICON_SIZE - CELL_MARGIN,
                             CELL_HEIGHT_2);
    draw_text(ctx, str_time, FONT_KEY_GOTHIC_24_BOLD, frame_time, GTextAlignmentRight);
    
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

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received) {
    uint32_t size = dict_size(received);
    printf("-=-=-=-%s\n%p %lu",__func__,received,size);
}

static void message_failed_callback(void) {
    
}

static void request_next_stations() {
    DictionaryIterator parameters;
    
    // Prepare parameters
    size_t tuple_count = 1;
    if (s_from_to->from != STATION_NON) {
        ++tuple_count;
    }
    if (s_from_to->to != STATION_NON) {
        ++tuple_count;
    }
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), STATION_CODE_LENGTH * tuple_count);
    uint8_t *dict_buffer = malloc(dict_size);
    dict_write_begin(&parameters, dict_buffer, dict_size);
    
    dict_write_uint8(&parameters, MESSAGE_KEY_TYPE, MESSAGE_TYPE_NEXT_TRAINS);
    
    if (s_from_to->from != STATION_NON) {
        char *data = malloc(STATION_CODE_LENGTH);
        stations_get_code(s_from_to->from, data, STATION_CODE_LENGTH);
        dict_write_data(&parameters, MESSAGE_KEY_CODE_FROM, (uint8_t *)data, STATION_CODE_LENGTH);
        free(data);
    }
    if (s_from_to->to != STATION_NON) {
        char *data = malloc(STATION_CODE_LENGTH);
        stations_get_code(s_from_to->to, data, STATION_CODE_LENGTH);
        dict_write_data(&parameters, MESSAGE_KEY_CODE_TO, (uint8_t *)data, STATION_CODE_LENGTH);
        free(data);
    }
    
    dict_write_end(&parameters);
    
    // Send message
    message_send(&parameters,
                 (MessageCallbacks){
                     .message_succeeded_callback = message_succeeded_callback,
                     .message_failed_callback = message_failed_callback
                 });
    
    free(dict_buffer);
}

// MARK: Menu layer callbacks

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return NEXT_TRAINS_SECTION_COUNT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == NEXT_TRAINS_SECTION_INFO) {
        return 1;
    } else if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        return (s_next_trains_count > 0)?s_next_trains_count:1;
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
    bool is_dark_theme = status_is_dark_theme();
    bool is_highlighed = is_dark_theme || is_selected;
    GColor text_color = (is_selected && !is_dark_theme)?curr_bg_color():curr_fg_color();
#else
    GColor text_color = curr_fg_color();
#endif
    
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        draw_next_trains_info(ctx,
                              cell_layer,
                              text_color
#ifdef PBL_COLOR
                              ,is_highlighed
#endif
                              );
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (s_next_trains_count > 0) {
            DataModelNextTrain next_train = s_next_trains[cell_index->row];
            
            char *str_terminus = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
            stations_get_name(next_train.terminus, str_terminus, STATION_NAME_MAX_LENGTH);
            
            draw_next_trains_cell(ctx, cell_layer,
                                  text_color,
#ifdef PBL_COLOR
                                  is_highlighed,
#endif
                                  next_train.mission_code,
                                  next_train.hour,
                                  str_terminus,
                                  next_train.platform);
            free(str_terminus);
        } else if (s_is_updating) {
            graphics_context_set_text_color(ctx, text_color);
            draw_cell_title(ctx, cell_layer, "Loading...");
        } else {
            graphics_context_set_text_color(ctx, text_color);
            draw_cell_title(ctx, cell_layer, "No train.");
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

static void selection_will_change_callback(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, void *callback_context) {
    if (s_next_trains_count == 0 && new_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        *new_index = old_index;
    }
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
    if (s_from_to->to != STATION_NON) {
        s_str_to = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(s_from_to->to, s_str_to, STATION_NAME_MAX_LENGTH);
    }
    
    // Demo data
    s_is_updating = true;
    s_next_trains_count = 5;
    s_next_trains = malloc(sizeof(DataModelNextTrain) * s_next_trains_count);
    
    strcpy(s_next_trains[0].number, "123456");
    strcpy(s_next_trains[0].mission_code, "EAPE");
    strcpy(s_next_trains[0].hour, "00:01");
    strcpy(s_next_trains[0].platform, "9");
    s_next_trains[0].terminus = 133;
    
    strcpy(s_next_trains[1].number, "123456");
    strcpy(s_next_trains[1].mission_code, "NOPE");
    strcpy(s_next_trains[1].hour, "00:04");
    strcpy(s_next_trains[1].platform, "27");
    s_next_trains[1].terminus = 310;
    
    strcpy(s_next_trains[2].number, "123456");
    strcpy(s_next_trains[2].mission_code, "TOCA");
    strcpy(s_next_trains[2].hour, "00:10");
    strcpy(s_next_trains[2].platform, "C");
    s_next_trains[2].terminus = 353;
    
    strcpy(s_next_trains[3].number, "123456");
    strcpy(s_next_trains[3].mission_code, "SEBO");
    strcpy(s_next_trains[3].hour, "00:13");
    strcpy(s_next_trains[3].platform, "A");
    s_next_trains[3].terminus = 393;
    
    strcpy(s_next_trains[4].number, "123456");
    strcpy(s_next_trains[4].mission_code, "FOPE");
    strcpy(s_next_trains[4].hour, "00:19");
    strcpy(s_next_trains[4].platform, "BL");
    s_next_trains[4].terminus = 259;
    
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
        .selection_will_change = (MenuLayerSelectionWillChangeCallback)selection_will_change_callback,
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
    NULL_FREE(s_str_from);
    NULL_FREE(s_str_to);
    NULL_FREE(s_from_to);
    NULL_FREE(s_next_trains);
    
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
    request_next_stations();
}

static void window_disappear(Window *window) {
    message_clear_callbacks();
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
