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
static ClickConfigProvider s_ccp_of_menu_layer;
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

#define UPDATE_TIME_FORMAT_INTERVAL 3750 // 3.75 seconds
static AppTimer *s_update_time_format_timer;
#define IDLE_TIMEOUT 300000 // 5 minutes
static AppTimer *s_idle_timer;

static char* s_train_number;
static StationIndex s_from_station;

static size_t s_train_details_list_count;
static DataModelTrainDetail *s_train_details_list;
static bool s_is_updating;

static bool s_show_relative_time;

// Forward declaration

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context);
static void restart_timers();
static void idle_timer_start();

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
            return "Check Timetable";
        case TRAIN_DETAINS_ACTIONS_FAV:
            return "Set Favorite";
        default:
            return "";
    }
}

static bool action_list_is_enabled_callback(size_t index) {
    switch (index) {
        case TRAIN_DETAINS_ACTIONS_FAV:
        {
            MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
            StationIndex selected_station_index = s_train_details_list[selected_index.row].station;
            if (s_from_station == selected_station_index) {
                return !fav_exists((Favorite){selected_station_index, STATION_NON});
            } else {
                return !fav_exists((Favorite){s_from_station, selected_station_index});
            }
        }
        case TRAIN_DETAINS_ACTIONS_TIMETABLE:
        default:
            return true;
    }
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    StationIndex selected_station_index = s_train_details_list[selected_index.row].station;
    switch (index) {
        case TRAIN_DETAINS_ACTIONS_TIMETABLE:
            window_stack_remove(action_list_window, false);
            push_next_trains_window((DataModelFromTo){selected_station_index, STATION_NON}, true);
            break;
        case TRAIN_DETAINS_ACTIONS_FAV:
            fav_add(s_from_station, selected_station_index);
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
            size_t offset = 0;
            for (size_t data_index = 0; data_index < TRAIN_DETAIL_KEY_COUNT && size_left > 0; ++data_index) {
                switch (data_index) {
                    case TRAIN_DETAIL_KEY_TIME:
                        if (size_left >= 4) {
                            s_train_details_list[index].time = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
                            offset = 5;
                        }
                        break;
                    case TRAIN_DETAIL_KEY_STATION:
                        if (size_left >= 2) {
                            s_train_details_list[index].station = (data[0] << 8) + data[1];
                            offset = 3;
                        }
                        break;
                    default:
                        offset = 0;
                        break;
                }
                
                size_left -= (uint16_t)offset;
                data += offset;
            }
        }
    }
    
    // Update UI
    s_is_updating = false;
    restart_timers();
    s_show_relative_time = false;
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
    size_t train_number_length = strlen(s_train_number);
    
    size_t tuple_count = 2;
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), train_number_length);
    uint8_t *dict_buffer = malloc(dict_size);
    dict_write_begin(&parameters, dict_buffer, dict_size);
    
    dict_write_uint8(&parameters, MESSAGE_KEY_REQUEST_TYPE, MESSAGE_TYPE_TRAIN_DETAILS);
    dict_write_data(&parameters, MESSAGE_KEY_REQUEST_TRAIN_NUMBER, (uint8_t *)s_train_number, train_number_length);

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

static void restart_timers() {
    update_time_format_timer_stop();
    update_time_format_timer_start();
}

static void idle_timer_callback(void *context) {
    window_stack_pop_all(false);
    push_main_menu_window(false);
}

static void idle_timer_start() {
    app_timer_register(IDLE_TIMEOUT, idle_timer_callback, NULL);
}

static void idle_timer_stop() {
    if(s_idle_timer) {
        app_timer_cancel(s_idle_timer);
        s_idle_timer = NULL;
    }
}

// MARK: Click Config Provider

static void button_up_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    if (selected_index.row == 0) {
        uint16_t num_rows = menu_layer_get_num_rows_callback(s_menu_layer, 0, NULL);
        menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, num_rows - 1), MenuRowAlignBottom, true);
    } else {
        menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, true);
    }
}

static void button_down_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    uint16_t num_rows = menu_layer_get_num_rows_callback(s_menu_layer, 0, NULL);
    if (selected_index.row == num_rows - 1) {
        menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, 0), MenuRowAlignTop, true);
    } else {
        menu_layer_set_selected_next(s_menu_layer, false, MenuRowAlignCenter, true);
    }
}

static void click_config_provider(void *context) {
    s_ccp_of_menu_layer(context);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, button_up_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30, button_down_handler);
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (s_is_updating) {
        return 1;
    } else {
        return (s_train_details_list_count > 0)?s_train_details_list_count:1;
    }
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT_2;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
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
        draw_centered_title(ctx, cell_layer, "Loading...", NULL,
#ifdef PBL_COLOR
                            GColorBlack
#else
                            text_color
#endif
                            );
    } else if (s_train_details_list_count > 0) {
        DataModelTrainDetail train_detail = s_train_details_list[cell_index->row];
        
        // Time
        char *str_time = calloc(TIME_STRING_LENGTH, sizeof(char));
        time_2_str(train_detail.time, str_time, TIME_STRING_LENGTH, s_show_relative_time);
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(train_detail.station, str_station, STATION_NAME_MAX_LENGTH);

        draw_station(ctx, cell_layer,
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
        draw_centered_title(ctx, cell_layer, "No train.", NULL,
#ifdef PBL_COLOR
                            GColorBlack
#else
                            text_color
#endif
                            );
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (!s_is_updating && s_train_details_list_count > 0) {
        action_list_present_with_callbacks((ActionListCallbacks) {
            .get_bar_color = (ActionListGetBarColorCallback)action_list_get_bar_color,
            .get_num_rows = (ActionListGetNumberOfRowsCallback)action_list_get_num_rows_callback,
            .get_default_selection = (ActionListGetDefaultSelectionCallback)action_list_get_default_selection_callback,
            .get_title = (ActionListGetTitleCallback)action_list_get_title_callback,
            .is_enabled = (ActionListIsEnabledCallback)action_list_is_enabled_callback,
            .select_click = (ActionListSelectCallback)action_list_select_callback
        });
    }
}

#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)

static int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 1;
}

static void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)  {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

static void menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context) {
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
    
    // Add status bar
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer);
#endif
    
    // Add menu layer
    int16_t status_bar_height = 0;
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
    status_bar_height = STATUS_BAR_LAYER_HEIGHT;
#endif
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + status_bar_height,
                                   window_bounds.size.w,
                                   window_bounds.size.h - status_bar_height);
    s_menu_layer = menu_layer_create(menu_layer_frame);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
    
    // Setup menu layer
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
    menu_layer_pad_bottom_enable(s_menu_layer, false);
#endif
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
        ,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)menu_layer_draw_background_callback
#endif
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    s_ccp_of_menu_layer = window_get_click_config_provider(window);
    window_set_click_config_provider_with_context(window, click_config_provider, s_menu_layer);
    
    // Add inverter layer for Aplite
#ifdef PBL_BW
    s_inverter_layer = inverter_layer_create(window_bounds);
#endif
    
    // Setup theme
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
    
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
    layer_destroy(s_status_bar_background_layer);
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
    idle_timer_start();
}

static void window_disappear(Window *window) {
    message_clear_callbacks();
    
    // Stop timer
    update_time_format_timer_stop();
    idle_timer_stop();
}

// MARK: Entry point

void push_train_details_window(char* train_number, StationIndex from_station, bool animated) {
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
    size_t train_number_length = strlen(train_number) + 1;
    s_train_number = calloc(train_number_length, sizeof(char));
    strncpy(s_train_number, train_number, train_number_length);
    
    s_from_station = from_station;
    
    // Reset some status
    s_train_details_list_count = 0;
    NULL_FREE(s_train_details_list);
    s_show_relative_time = false;
    
    window_stack_push(s_window, animated);
}
