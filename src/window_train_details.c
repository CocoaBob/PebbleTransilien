//
//  window_train_details.c
//  PebbleTransilien
//
//  Created by CocoaBob on 23/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

typedef struct {
    Window *window;
    MenuLayer *menu_layer;
    
    ClickConfigProvider last_ccp;
    
#ifdef PBL_BW
    InverterLayer *inverter_layer;
#endif
    
#if !defined(PBL_PLATFORM_APLITE)
#define UPDATE_TIME_FORMAT_INTERVAL 3000 // 3 seconds
    AppTimer *format_timer;
#endif
    
    char* train_number;
    StationIndex from_station;
    
    size_t train_details_list_count;
    DataModelTrainDetail *train_details_list;
    bool is_updating;
    
    bool show_relative_time;
} TrainDetails;

// Forward declaration

#if !defined(PBL_PLATFORM_APLITE)
static void restart_timers(TrainDetails *user_info);
#endif

// MARK: Click Config Provider

static void click_config_provider(void *context) {
    MenuLayer *menu_layer = context;
    TrainDetails *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
    
    user_info->last_ccp(user_info->menu_layer);
    
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, common_menu_layer_button_up_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30, common_menu_layer_button_down_handler);
}

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received, TrainDetails *user_info) {
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (tuple_type->value->int8 != MESSAGE_TYPE_TRAIN_DETAILS) {
        return;
    }
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    size_t count = tuple_payload_count->value->int16;
    
    NULL_FREE(user_info->train_details_list);
    user_info->train_details_list_count = count;
    if (user_info->train_details_list_count > 0) {
        user_info->train_details_list = malloc(sizeof(DataModelTrainDetail) * user_info->train_details_list_count);
    }
    
    for (size_t idx = 0; idx < count; ++idx) {
        Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + idx);
        if (tuple_payload->type == TUPLE_BYTE_ARRAY) {
            uint8_t *data = tuple_payload->value->data;
            uint16_t size_left = tuple_payload->length;
            size_t str_length = 0,offset = 0;
            for (size_t data_index = 0; data_index < TRAIN_DETAIL_KEY_COUNT && size_left > 0; ++data_index) {
                data += offset;
                str_length = (data_index == TRAIN_DETAIL_KEY_TIME)?4:strlen((char *)data);
                offset = str_length + 1;
                
                long long temp_int = 0;
                for (size_t i = 0; i < str_length; ++i) {
                    temp_int += data[i] << (8 * (str_length - i - 1));
                }
                if (data_index == TRAIN_DETAIL_KEY_TIME) {
                    user_info->train_details_list[idx].time = temp_int;
                } else if (data_index == TRAIN_DETAIL_KEY_STATION) {
                    user_info->train_details_list[idx].station = temp_int;
                }
                
                size_left -= (uint16_t)offset;
            }
        }
    }
    
    // Update UI
    user_info->is_updating = false;
#if !defined(PBL_PLATFORM_APLITE)
    restart_timers(user_info);
#endif
    user_info->show_relative_time = false;
    menu_layer_reload_data(user_info->menu_layer);
    vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t[]) {50}, .num_segments = 1});
}

static void message_failed_callback(TrainDetails *user_info) {
    // TODO: Cancel loading...
}

static void request_train_details(TrainDetails *user_info) {
    // Update UI
    user_info->is_updating = true;
    user_info->train_details_list_count = 0;
    NULL_FREE(user_info->train_details_list);
    menu_layer_reload_data(user_info->menu_layer);
    
    // Prepare parameters
    DictionaryIterator parameters;
    size_t train_number_length = strlen(user_info->train_number);
    
    size_t tuple_count = 2;
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), train_number_length);
    uint8_t *dict_buffer = malloc(dict_size);
    
    dict_write_begin(&parameters, dict_buffer, dict_size);
    
    dict_write_uint8(&parameters, MESSAGE_KEY_REQUEST_TYPE, MESSAGE_TYPE_TRAIN_DETAILS);
    dict_write_data(&parameters, MESSAGE_KEY_REQUEST_TRAIN_NUMBER, (uint8_t *)user_info->train_number, train_number_length);

    dict_write_end(&parameters);
    
    // Send message
    message_send(&parameters,
                 (MessageCallbacks){
                     .message_succeeded_callback = (MessageSucceededCallback)message_succeeded_callback,
                     .message_failed_callback = (MessageFailedCallback)message_failed_callback
                 },
                 user_info);
    
    free(dict_buffer);
}

#if !defined(PBL_PLATFORM_APLITE)
// MARK: Timers

static void format_timer_callback(TrainDetails *user_info);

static void format_timer_start(TrainDetails *user_info) {
    user_info->format_timer = app_timer_register(UPDATE_TIME_FORMAT_INTERVAL, (AppTimerCallback)format_timer_callback, user_info);
}

static void format_timer_stop(TrainDetails *user_info) {
    if(user_info->format_timer) {
        app_timer_cancel(user_info->format_timer);
    }
}

static void format_timer_callback(TrainDetails *user_info) {
    user_info->show_relative_time = !user_info->show_relative_time;
    layer_mark_dirty(menu_layer_get_layer(user_info->menu_layer));
    format_timer_start(user_info);
}

static void restart_timers(TrainDetails *user_info) {
    format_timer_stop(user_info);
    format_timer_start(user_info);
}
#endif

// MARK: Accel Tap Service

static void accel_tap_service_handler(AccelAxisType axis, int32_t direction, void *context) {
    request_train_details(context);
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, TrainDetails *user_info) {
    if (user_info->is_updating) {
        return 1;
    } else {
        return (user_info->train_details_list_count > 0)?user_info->train_details_list_count:1;
    }
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, TrainDetails *user_info) {
    return CELL_HEIGHT_2;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, TrainDetails *user_info) {
    MenuIndex selected_index = menu_layer_get_selected_index(user_info->menu_layer);
    bool is_selected = selected_index.row == cell_index->row;
#ifdef PBL_COLOR
    bool is_highlighed = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#endif
    
    if (user_info->is_updating) {
        draw_centered_title(ctx, cell_layer,
                            is_selected,
                            _("Loading..."),
                            NULL);
    } else if (user_info->train_details_list_count > 0) {
        DataModelTrainDetail train_detail = user_info->train_details_list[cell_index->row];
        
        // Time
        char *str_time = calloc(TIME_STRING_LENGTH, sizeof(char));
        time_2_str(train_detail.time, str_time, TIME_STRING_LENGTH, user_info->show_relative_time);
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(train_detail.station, str_station, STATION_NAME_MAX_LENGTH);

        draw_station(ctx, cell_layer,
#if !defined(PBL_PLATFORM_APLITE)
                     menu_layer_get_layer(user_info->menu_layer), is_selected,
#endif
#ifdef PBL_COLOR
                     text_color,
                     is_highlighed,
#else
                     false,
#endif
                     str_time,
                     str_station);
        
        // Clean
        free(str_time);
        free(str_station);
    } else {
        draw_centered_title(ctx, cell_layer,
                            is_selected,
                            _("No train."),
                            NULL);
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, TrainDetails *user_info) {
    MenuIndex selected_index = menu_layer_get_selected_index(user_info->menu_layer);
    if (ui_can_push_window()) {
        ui_push_window(new_window_next_trains((DataModelFromTo){user_info->train_details_list[selected_index.row].station, STATION_NON}));
    }
}

#if !defined(PBL_PLATFORM_APLITE)
static void menu_layer_selection_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) {
    text_scroll_end();
}
#endif

// MARK: Window callbacks

static void window_load(Window *window) {
    TrainDetails *user_info = window_get_user_data(window);
    
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + STATUS_BAR_LAYER_HEIGHT,
                                   window_bounds.size.w,
                                   window_bounds.size.h - STATUS_BAR_LAYER_HEIGHT);
    user_info->menu_layer = menu_layer_create(menu_layer_frame);
    layer_add_child(window_layer, menu_layer_get_layer(user_info->menu_layer));
    
    // Setup menu layer
#if !defined(PBL_PLATFORM_APLITE)
    menu_layer_pad_bottom_enable(user_info->menu_layer, false);
#endif
    menu_layer_set_callbacks(user_info->menu_layer, user_info, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback
#if !defined(PBL_PLATFORM_APLITE)
        ,
        .selection_changed = (MenuLayerSelectionChangedCallback)menu_layer_selection_changed,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)common_menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)common_menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)common_menu_layer_draw_background_callback
#endif
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(user_info->menu_layer, window);
    user_info->last_ccp = window_get_click_config_provider(window);
    window_set_click_config_provider_with_context(window, click_config_provider, user_info->menu_layer);
    
    // Add inverter layer for Aplite
#ifdef PBL_BW
    user_info->inverter_layer = inverter_layer_create(window_bounds);
#endif
    
    // Setup theme
#ifdef PBL_COLOR
    ui_setup_theme(user_info->window, user_info->menu_layer);
#else
    ui_setup_theme(user_info->window, user_info->inverter_layer);
#endif
}

static void window_unload(Window *window) {
    TrainDetails *user_info = window_get_user_data(window);
    
    // Data
    NULL_FREE(user_info->train_number);
    NULL_FREE(user_info->train_details_list);
    
    // Window
    menu_layer_destroy(user_info->menu_layer);
    window_destroy(user_info->window);
    
#ifdef PBL_BW
    inverter_layer_destroy(user_info->inverter_layer);
#endif
    
    NULL_FREE(user_info);
}

static void window_appear(Window *window) {
    // Discard formerly sent requests
    message_clear_callbacks();
    
    TrainDetails *user_info = window_get_user_data(window);
    
    // Add status bar
    ui_setup_status_bar(window_get_root_layer(user_info->window), menu_layer_get_layer(user_info->menu_layer));
    
    // Subscribe services
    accel_tap_service_init(accel_tap_service_handler, user_info);
    
    // Request data
    if (user_info->train_details_list == NULL) {
        request_train_details(user_info);
    }
    
    // Start UI timer
#if !defined(PBL_PLATFORM_APLITE)
    format_timer_start(user_info);
#endif
    
}

static void window_disappear(Window *window) {
    // Set callbacks to NULL
    message_clear_callbacks();
    
#if !defined(PBL_PLATFORM_APLITE)
    // Stop scrolling text
    text_scroll_end();
#endif
    
    // Unsubscribe services
    accel_tap_service_deinit();
    
#if !defined(PBL_PLATFORM_APLITE)
    TrainDetails *user_info = window_get_user_data(window);
    // Stop UI timer
    format_timer_stop(user_info);
#endif
}

// MARK: Entry point

Window* new_window_train_details(char* train_number, StationIndex from_station) {
    TrainDetails *user_info = calloc(1, sizeof(TrainDetails));
    if (user_info) {
        user_info->window = window_create();
        window_set_user_data(user_info->window, user_info);
        window_set_window_handlers(user_info->window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
        
        size_t train_number_length = strlen(train_number) + 1;
        user_info->train_number = calloc(train_number_length, sizeof(char));
        strncpy(user_info->train_number, train_number, train_number_length);
        
        user_info->from_station = from_station;
        
#ifdef PBL_SDK_2
        // Fullscreen
        window_set_fullscreen(user_info->window, true);
#endif
        
        // Return the window
        return user_info->window;
    }
    return NULL;
}
