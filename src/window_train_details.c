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
    
#if RELATIVE_TIME_IS_ENABLED
#define UPDATE_TIME_FORMAT_INTERVAL 3000 // 3 seconds
    AppTimer *format_timer;
    bool show_relative_time;
#endif
    
    char* train_number;
    StationIndex from_station;
    
    int train_details_list_count;
    DataModelTrainDetail *train_details_list;
    bool is_updating;
    
#if TEXT_SCROLL_IS_ENABLED
    TextScrollData *text_scroll_data;
#endif
} TrainDetails;

// Forward declaration

#if RELATIVE_TIME_IS_ENABLED
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

static void message_callback(bool succeeded, TrainDetails *user_info, MESSAGE_TYPE type, ...) {
    if (succeeded && type == MESSAGE_TYPE_TRAIN_DETAILS) {
        NULL_FREE(user_info->train_details_list);
        
        va_list ap;
        va_start(ap, type);
        
        user_info->train_details_list = va_arg(ap, void *);
        user_info->train_details_list_count = va_arg(ap, size_t);
        
        va_end(ap);
        
        // Update UI
#if RELATIVE_TIME_IS_ENABLED
        restart_timers(user_info);
        user_info->show_relative_time = false;
#endif
    } else {
        user_info->train_details_list_count = -1;
    }
    user_info->is_updating = false;
    menu_layer_reload_data(user_info->menu_layer);
    
    // Feedback
    vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t[]) {50}, .num_segments = 1});
}

static void request_train_details(TrainDetails *user_info) {
    // Update UI
    user_info->is_updating = true;
    user_info->train_details_list_count = 0;
    NULL_FREE(user_info->train_details_list);
    menu_layer_reload_data(user_info->menu_layer);
    
    size_t train_number_length = strlen(user_info->train_number);
    
    // Send message
    message_send(MESSAGE_TYPE_TRAIN_DETAILS,
                 (MessageCallback)message_callback,
                 user_info,
                 (uint8_t *)user_info->train_number,
                 train_number_length);
}

// MARK: Timers
#if RELATIVE_TIME_IS_ENABLED

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
    return (user_info->train_details_list_count > 0)?user_info->train_details_list_count:1;
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, TrainDetails *user_info) {
    return CELL_HEIGHT_2;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, TrainDetails *user_info) {
    bool is_selected = menu_layer_is_index_selected(user_info->menu_layer, cell_index);
    
#ifdef PBL_COLOR
    bool is_inverted = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#else
    bool is_inverted = settings_is_dark_theme()?!is_selected:is_selected;
    GColor text_color = is_selected?curr_bg_color():curr_fg_color();
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
        time_2_str(train_detail.time,
                   str_time,
                   TIME_STRING_LENGTH
#if RELATIVE_TIME_IS_ENABLED
                   ,
                   user_info->show_relative_time
#endif
                   );
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(train_detail.station, str_station, STATION_NAME_MAX_LENGTH);

        draw_station(ctx, cell_layer,
#if TEXT_SCROLL_IS_ENABLED
                     &user_info->text_scroll_data, menu_layer_get_layer(user_info->menu_layer), is_selected,
#endif
                     text_color,
                     is_inverted,
                     str_time,
                     str_station);
        
        // Clean
        free(str_time);
        free(str_station);
    } else if (user_info->train_details_list_count == 0) {
        draw_centered_title(ctx, cell_layer,
                            is_selected,
                            _("No train."),
                            NULL);
    } else if (user_info->train_details_list_count == -1) {
        draw_centered_title(ctx, cell_layer,
                            is_selected,
                            _("Request Failed."),
                            NULL);
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, TrainDetails *user_info) {
    MenuIndex selected_index = menu_layer_get_selected_index(user_info->menu_layer);
    ui_push_window(new_window_next_trains((DataModelFromTo){user_info->train_details_list[selected_index.row].station, STATION_NON}));
}

#if TEXT_SCROLL_IS_ENABLED
static void menu_layer_selection_will_change(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, TrainDetails *user_info) {
    text_scroll_destory(user_info->text_scroll_data);
    user_info->text_scroll_data = NULL;
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
#ifdef PBL_ROUND
    menu_layer_frame.size.h -= STATUS_BAR_LAYER_HEIGHT;
#endif
    user_info->menu_layer = menu_layer_create(menu_layer_frame);
#ifdef PBL_ROUND
    menu_layer_set_center_focused(user_info->menu_layer, false);
#endif
    layer_add_child(window_layer, menu_layer_get_layer(user_info->menu_layer));
    
    // Setup menu layer
    menu_layer_pad_bottom_enable(user_info->menu_layer, false);
    menu_layer_set_callbacks(user_info->menu_layer, user_info, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback,
#if TEXT_SCROLL_IS_ENABLED
        .selection_will_change = (MenuLayerSelectionWillChangeCallback)menu_layer_selection_will_change,
#endif
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)common_menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)common_menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)common_menu_layer_draw_background_callback
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(user_info->menu_layer, window);
    user_info->last_ccp = window_get_click_config_provider(window);
    window_set_click_config_provider_with_context(window, click_config_provider, user_info->menu_layer);
    
    // Setup theme
    ui_setup_theme(user_info->menu_layer);
}

static void window_unload(Window *window) {
    TrainDetails *user_info = window_get_user_data(window);
    
    // Data
    NULL_FREE(user_info->train_number);
    NULL_FREE(user_info->train_details_list);
    
    // Window
    menu_layer_destroy(user_info->menu_layer);
    window_destroy(user_info->window);
    
    NULL_FREE(user_info);
}

static void window_appear(Window *window) {
    // Discard formerly sent requests
    message_clear_callbacks();
    
    TrainDetails *user_info = window_get_user_data(window);
    
    // Add status bar
    ui_setup_status_bars(window_get_root_layer(user_info->window), menu_layer_get_layer(user_info->menu_layer));
    
    // Subscribe services
    accel_tap_service_init(accel_tap_service_handler, user_info);
    
    // Request data
    if (user_info->train_details_list == NULL) {
        request_train_details(user_info);
    }
    
    // Start timers
#if RELATIVE_TIME_IS_ENABLED
    format_timer_start(user_info);
#endif
    
}

static void window_disappear(Window *window) {
#if TEXT_SCROLL_IS_ENABLED || RELATIVE_TIME_IS_ENABLED
    TrainDetails *user_info = window_get_user_data(window);
#endif
    
    // Set callbacks to NULL
    message_clear_callbacks();
    
#if TEXT_SCROLL_IS_ENABLED
    // Stop scrolling text
    text_scroll_destory(user_info->text_scroll_data);
    user_info->text_scroll_data = NULL;
#endif
    
    // Unsubscribe services
    accel_tap_service_deinit();
    
    // Stop timers
#if RELATIVE_TIME_IS_ENABLED
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
        
        // Return the window
        return user_info->window;
    }
    return NULL;
}
