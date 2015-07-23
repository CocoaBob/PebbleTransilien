//
//  train_details_window.c
//  PebbleTransilien
//
//  Created by CocoaBob on 23/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static Window *s_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_PLATFORM_BASALT
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
static Layer *s_status_bar_overlay_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

static char* s_train_number;

static size_t s_train_details_count;
static DataModelTrainDetail *s_train_details;
static bool s_is_updating;

// MARK: Constants

#define TRAIN_DETAIL_CELL_TIME_W 36     // CELL_MARGIN + TRAIN_DETAIL_CELL_CODE_W + CELL_MARGIN = 4 + 56

// MARK: Drawing

void draw_train_detail_cell(GContext *ctx, Layer *cell_layer,
                            GColor text_color,
#ifdef PBL_COLOR
                            bool is_highlighed,
#endif
                            const char * str_time,
                            const char * str_station) {
    graphics_context_set_text_color(ctx, text_color);
    GRect bounds = layer_get_bounds(cell_layer);
    
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT_2 - FROM_TO_ICON_WIDTH) / 2,
                             FROM_TO_ICON_WIDTH,
                             FROM_TO_ICON_WIDTH);
#ifdef PBL_COLOR
    draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
#else
    draw_image_in_rect(ctx, RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
#endif
    
    // Station
    GRect frame_station = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                                TEXT_Y_OFFSET,
                                bounds.size.w - CELL_MARGIN * 4 - TRAIN_DETAIL_CELL_TIME_W - FROM_TO_ICON_WIDTH,
                                CELL_HEIGHT_2);
    draw_text(ctx, str_station, FONT_KEY_GOTHIC_18, frame_station, GTextAlignmentLeft);
    
    // Time
    GRect frame_time = GRect(bounds.size.w - CELL_MARGIN - TRAIN_DETAIL_CELL_TIME_W,
                             TEXT_Y_OFFSET,
                             TRAIN_DETAIL_CELL_TIME_W,
                             CELL_HEIGHT_2);
    draw_text(ctx, str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time, GTextAlignmentRight);
}

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received) {
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (tuple_type->value->int8 != MESSAGE_TYPE_TRAIN_DETAILS) {
        return;
    }
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    size_t count = tuple_payload_count->value->int16;
    
    s_train_details_count = count;
    NULL_FREE(s_train_details);
    s_train_details = malloc(sizeof(DataModelTrainDetail) * s_train_details_count);
    
    for (uint32_t index = 0; index < count; ++index) {
        Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + index);
        if (tuple_payload->type == TUPLE_BYTE_ARRAY) {
            uint8_t *data = tuple_payload->value->data;
            uint16_t size_left = tuple_payload->length;
            size_t str_length = strlen((char *)data);
            size_t offset = str_length + 1;
            for (size_t data_index = 0; data_index < TRAIN_DETAIL_KEY_COUNT && size_left > 0; ++data_index) {
                switch (data_index) {
                    case TRAIN_DETAIL_KEY_TIME:
                        strcpy(s_train_details[index].time, (char *)data);
                        break;
                    case TRAIN_DETAIL_KEY_STATION:
                        s_train_details[index].station = (data[0] << 8) + data[1];
                        break;
                    default:
                        break;
                }
                
                size_left -= (uint16_t)offset;
                data += offset;
                if (data_index == TRAIN_DETAIL_KEY_STATION) {
                    str_length = 2;
                } else {
                    str_length = strlen((char *)data);
                }
                offset = str_length + 1;
            }
        }
    }
    
    // Update UI
    s_is_updating = false;
    menu_layer_reload_data(s_menu_layer);
}

static void message_failed_callback(void) {
    // TODO: Cancel loading...
}

static void request_train_details() {
    // Update UI
    s_is_updating = true;
    s_train_details_count = 0;
    menu_layer_reload_data(s_menu_layer);
    
    // Prepare parameters
    DictionaryIterator parameters;
    
    size_t tuple_count = 1;
//    if (s_from_to->from != STATION_NON) {
//        ++tuple_count;
//    }
//    if (s_from_to->to != STATION_NON) {
//        ++tuple_count;
//    }
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), STATION_CODE_LENGTH * tuple_count);
    uint8_t *dict_buffer = malloc(dict_size);
    dict_write_begin(&parameters, dict_buffer, dict_size);
    
    dict_write_uint8(&parameters, MESSAGE_KEY_REQUEST_TYPE, MESSAGE_TYPE_TRAIN_DETAILS);
    
//    if (s_from_to->from != STATION_NON) {
//        char *data = malloc(STATION_CODE_LENGTH);
//        stations_get_code(s_from_to->from, data, STATION_CODE_LENGTH);
//        dict_write_data(&parameters, MESSAGE_KEY_REQUEST_CODE_FROM, (uint8_t *)data, STATION_CODE_LENGTH);
//        free(data);
//    }
//    if (s_from_to->to != STATION_NON) {
//        char *data = malloc(STATION_CODE_LENGTH);
//        stations_get_code(s_from_to->to, data, STATION_CODE_LENGTH);
//        dict_write_data(&parameters, MESSAGE_KEY_REQUEST_CODE_TO, (uint8_t *)data, STATION_CODE_LENGTH);
//        free(data);
//    }
    
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

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    return (s_train_details_count > 0)?s_train_details_count:1;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT_2;
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
    
    if (s_train_details_count > 0) {
        DataModelTrainDetail train_detail = s_train_details[cell_index->row];
        
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(train_detail.station, str_station, STATION_NAME_MAX_LENGTH);
        
        draw_train_detail_cell(ctx, cell_layer,
                               text_color,
#ifdef PBL_COLOR
                               is_highlighed,
#endif
                               train_detail.time,
                               str_station);
        free(str_station);
    } else if (s_is_updating) {
        graphics_context_set_text_color(ctx, text_color);
        draw_cell_title(ctx, cell_layer, "Loading...");
    } else {
        graphics_context_set_text_color(ctx, text_color);
        draw_cell_title(ctx, cell_layer, "No train.");
    }
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
    // Demo data
    s_is_updating = true;
    s_train_details_count = 6;
    s_train_details = malloc(sizeof(DataModelTrainDetail) * s_train_details_count);
    
    strcpy(s_train_details[0].time, "23:58");
    s_train_details[0].station = 354;
    
    strcpy(s_train_details[1].time, "00:01");
    s_train_details[1].station = 356;
    
    strcpy(s_train_details[2].time, "00:04");
    s_train_details[2].station = 86;
    
    strcpy(s_train_details[3].time, "00:07");
    s_train_details[3].station = 3;
    
    strcpy(s_train_details[4].time, "00:08");
    s_train_details[4].station = 23;
    
    strcpy(s_train_details[5].time, "00:11");
    s_train_details[5].station = 279;
    
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
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)draw_separator_callback
#ifdef PBL_PLATFORM_BASALT
        ,
        .draw_background = (MenuLayerDrawBackgroundCallback)draw_background_callback
#endif
    });
    
    // Finally, add status bar
#ifdef PBL_PLATFORM_BASALT
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer, &s_status_bar_overlay_layer);
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
    NULL_FREE(s_train_number);
    NULL_FREE(s_train_details);
    
    // Window
    menu_layer_destroy(s_menu_layer);
    window_destroy(window);
    s_window = NULL;
    
#ifdef PBL_PLATFORM_BASALT
    layer_destroy(s_status_bar_background_layer);
    layer_destroy(s_status_bar_overlay_layer);
    status_bar_layer_destroy(s_status_bar);
#endif
    
#ifdef PBL_BW
    inverter_layer_destroy(s_inverter_layer);
#endif
}

static void window_appear(Window *window) {
//    request_train_details();
}

static void window_disappear(Window *window) {
    message_clear_callbacks();
}

// MARK: Entry point

void push_train_details_window(char train_number[7]) {
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
    }
    
    NULL_FREE(s_train_number);
    s_train_number = malloc(sizeof(char) * 7);
    strncpy(s_train_number, train_number, 7);
    
    window_stack_push(s_window, true);
}
