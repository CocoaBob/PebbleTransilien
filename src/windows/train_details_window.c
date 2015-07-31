//
//  train_details_window.c
//  PebbleTransilien
//
//  Created by CocoaBob on 23/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

enum {
    TRAIN_DETAINS_ACTIONS_TIMETABLE = 0,
    TRAIN_DETAINS_ACTIONS_FAV,
    TRAIN_DETAINS_ACTIONS_COUNT
};

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

#define UPDATE_TIME_FORMAT_INTERVAL 3000 // 2 seconds
static AppTimer *s_update_time_format_timer;

static char* s_train_number;

static size_t s_train_details_list_count;
static DataModelTrainDetail *s_train_details_list;
static bool s_is_updating;

static bool s_show_relative_time;

// MARK: Drawing

static void draw_menu_layer_cell(GContext *ctx, Layer *cell_layer,
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
    
    // Time
    GRect frame_time = GRect(0,
                             TEXT_Y_OFFSET,
                             bounds.size.w,
                             CELL_HEIGHT_2);
    
    GSize time_size = graphics_text_layout_get_content_size(str_time,
                                                            fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                                                            frame_time,
                                                            GTextOverflowModeTrailingEllipsis,
                                                            GTextAlignmentRight);
    frame_time.origin.x = bounds.size.w - CELL_MARGIN - time_size.w;
    frame_time.size.w = time_size.w;
    draw_text(ctx, str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time, GTextAlignmentRight);
    
    // Station
    GRect frame_station = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                                TEXT_Y_OFFSET,
                                bounds.size.w - CELL_MARGIN * 4 - time_size.w - FROM_TO_ICON_WIDTH,
                                CELL_HEIGHT_2);
    draw_text(ctx, str_station, FONT_KEY_GOTHIC_18, frame_station, GTextAlignmentLeft);
}

// MARK: Action list callbacks

static GColor action_list_get_bar_color(void) {
#ifdef PBL_COLOR
    return GColorCobaltBlue;
#else
    return GColorWhite;
#endif
}

static size_t action_list_get_num_rows_callback(void) {
    return TRAIN_DETAINS_ACTIONS_COUNT;
}

static size_t action_list_get_default_selection_callback(void) {
    return TRAIN_DETAINS_ACTIONS_TIMETABLE;
}

static char* action_list_get_title_callback(size_t index) {
    switch (index) {
        case TRAIN_DETAINS_ACTIONS_TIMETABLE:
            return "Timetable";
            break;
        case TRAIN_DETAINS_ACTIONS_FAV:
            return "Favorite";
            break;
        default:
            return "";
            break;
    }
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    StationIndex selected_station_index = s_train_details_list[selected_index.row].station;
    switch (index) {
        case TRAIN_DETAINS_ACTIONS_TIMETABLE:
            window_stack_pop_all(false); // Pop Next Trains
            push_main_menu_window(false);
            push_next_trains_window((DataModelFromTo){selected_station_index, STATION_NON}, true);
            break;
        case TRAIN_DETAINS_ACTIONS_FAV:
            // TODO:
            fav_add(selected_station_index, STATION_NON);
            window_stack_remove(action_list_window, true);
            break;
        default:
            break;
    }
}

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received) {
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (tuple_type->value->int8 != MESSAGE_TYPE_TRAIN_DETAILS) {
        return;
    }
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    size_t count = tuple_payload_count->value->int16;
    
    s_train_details_list_count = count;
    NULL_FREE(s_train_details_list);
    s_train_details_list = malloc(sizeof(DataModelTrainDetail) * s_train_details_list_count);
    
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
                        if (size_left >= 4) {
                            s_train_details_list[index].time = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
                        }
                        break;
                    case TRAIN_DETAIL_KEY_STATION:
                        if (size_left >= 2) {
                            s_train_details_list[index].station = (data[0] << 8) + data[1];
                        }
                        break;
                    default:
                        break;
                }
                
                size_left -= (uint16_t)offset;
                data += offset;
                str_length = strlen((char *)data);
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
    s_train_details_list_count = 0;
    menu_layer_reload_data(s_menu_layer);
    
    // Prepare parameters
    DictionaryIterator parameters;
    
    size_t tuple_count = 2;
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), TRAIN_NUMBER_LENGTH);
    uint8_t *dict_buffer = malloc(dict_size);
    dict_write_begin(&parameters, dict_buffer, dict_size);
    
    dict_write_uint8(&parameters, MESSAGE_KEY_REQUEST_TYPE, MESSAGE_TYPE_TRAIN_DETAILS);
    dict_write_data(&parameters, MESSAGE_KEY_REQUEST_TRAIN_NUMBER, (uint8_t *)s_train_number, TRAIN_NUMBER_LENGTH);
    
    dict_write_end(&parameters);
    
    // Send message
    message_send(&parameters,
                 (MessageCallbacks){
                     .message_succeeded_callback = message_succeeded_callback,
                     .message_failed_callback = message_failed_callback
                 });
    
    free(dict_buffer);
}

// MARK: Timer

static void update_time_format_timer_callback(void *context);

static void update_time_format_timer_start() {
    s_update_time_format_timer = app_timer_register(UPDATE_TIME_FORMAT_INTERVAL, update_time_format_timer_callback, NULL);
}

static void update_time_format_timer_stop() {
    if(s_update_time_format_timer) {
        app_timer_cancel(s_update_time_format_timer);
        s_update_time_format_timer = NULL;
    }
}

static void update_time_format_timer_callback(void *context) {
    s_show_relative_time = !s_show_relative_time;
    menu_layer_reload_data(s_menu_layer);
    update_time_format_timer_start();
}

// MARK: Menu layer callbacks

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (s_is_updating) {
        return 1;
    } else {
        return (s_train_details_list_count > 0)?s_train_details_list_count:1;
    }
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
    
    if (s_is_updating) {
        graphics_context_set_text_color(ctx, text_color);
        draw_cell_title(ctx, cell_layer, "Loading...");
    } else if (s_train_details_list_count > 0) {
        DataModelTrainDetail train_detail = s_train_details_list[cell_index->row];
        
        // Time
        char *str_time = calloc(TIME_STRING_LENGTH, sizeof(char));
        time_2_str(train_detail.time, str_time, TIME_STRING_LENGTH, s_show_relative_time);
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(train_detail.station, str_station, STATION_NAME_MAX_LENGTH);

        draw_menu_layer_cell(ctx, cell_layer,
                             text_color,
#ifdef PBL_COLOR
                             is_highlighed,
#endif
                             str_time,
                             str_station);
        
        // Clean
        free(str_time);
        free(str_station);
    } else {
        graphics_context_set_text_color(ctx, text_color);
        draw_cell_title(ctx, cell_layer, "No train.");
    }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    action_list_present_with_callbacks((ActionListCallbacks) {
        .get_bar_color = (ActionListGetBarColorCallback)action_list_get_bar_color,
        .get_num_rows = (ActionListGetNumberOfRowsCallback)action_list_get_num_rows_callback,
        .get_default_selection = (ActionListGetDefaultSelectionCallback)action_list_get_default_selection_callback,
        .get_title = (ActionListGetTitleCallback)action_list_get_title_callback,
        .select_click = (ActionListSelectCallback)action_list_select_callback
    });
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
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    int16_t status_bar_height = 0;
#ifdef PBL_PLATFORM_BASALT
    status_bar_height = STATUS_BAR_LAYER_HEIGHT;
#endif
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + status_bar_height,
                                   window_bounds.size.w,
                                   window_bounds.size.h - status_bar_height);
    s_menu_layer = menu_layer_create(menu_layer_frame);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
    // Setup menu layer
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
        .select_click = (MenuLayerSelectCallback)select_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)draw_separator_callback
#ifdef PBL_PLATFORM_BASALT
        ,
        .draw_background = (MenuLayerDrawBackgroundCallback)draw_background_callback
#endif
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    
    // Finally, add status bar
#ifdef PBL_PLATFORM_BASALT
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer, &s_status_bar_overlay_layer);
#endif
    
#ifdef PBL_BW
    s_inverter_layer = inverter_layer_create(window_bounds);
#endif
    
#ifdef PBL_COLOR
    setup_ui_theme(s_window, s_menu_layer);
#else
    setup_ui_theme(s_window, s_inverter_layer);
#endif
}

static void window_unload(Window *window) {
    // Data
    NULL_FREE(s_train_number);
    NULL_FREE(s_train_details_list);
    
    // Window
    menu_layer_destroy(s_menu_layer);
    window_destroy(s_window);
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
    if (s_train_details_list == NULL) {
        request_train_details();
    }
    
    // Start timer
    update_time_format_timer_start();
}

static void window_disappear(Window *window) {
    message_clear_callbacks();
    
    // Stop timer
    update_time_format_timer_stop();
}

// MARK: Entry point

void push_train_details_window(char train_number[7], bool animated) {
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
    s_train_number = malloc(sizeof(char) * TRAIN_NUMBER_LENGTH);
    strncpy(s_train_number, train_number, TRAIN_NUMBER_LENGTH);
    
    // Reset some status
    s_train_details_list_count = 0;
    NULL_FREE(s_train_details_list);
    s_show_relative_time = false;
    
    window_stack_push(s_window, animated);
}
