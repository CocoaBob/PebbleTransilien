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

enum {
    NEXT_TRAINS_ACTIONS_FAV = 0,
    NEXT_TRAINS_ACTIONS_COUNT
};

static Window *s_window;
static MenuLayer *s_menu_layer;
static ClickConfigProvider s_last_ccp;
#ifdef PBL_PLATFORM_BASALT
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

#define RELOAD_DATA_TIMER_INTERVAL 15000 // 15 seconds
static AppTimer *s_reload_data_timer;
#define UPDATE_TIME_FORMAT_INTERVAL 3000 // 2 seconds
static AppTimer *s_update_time_format_timer;

static DataModelFromTo s_from_to;
static char *s_str_from;
static char *s_str_to;

static size_t s_next_trains_list_count;
static DataModelNextTrain *s_next_trains_list;
static bool s_is_updating;

static bool s_show_relative_time;

// Forward declaration

static void restart_timers();

// MARK: Constants

#define NEXT_TRAIN_CELL_ICON_Y 4    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_X 4    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_W 56
#define NEXT_TRAIN_CELL_TIME_X 64   // CELL_MARGIN + NEXT_TRAIN_CELL_CODE_W + CELL_MARGIN = 4 + 56 + 4

// MARK: Drawing

static void draw_menu_layer_cell(GContext *ctx, Layer *cell_layer,
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
                             TEXT_Y_OFFSET - 2,
                             NEXT_TRAIN_CELL_CODE_W,
                             CELL_HEIGHT_2);
    draw_text(ctx, str_code, FONT_KEY_GOTHIC_24_BOLD, frame_code, GTextAlignmentLeft);
    
    // Time
    GRect frame_time = GRect(NEXT_TRAIN_CELL_TIME_X,
                             TEXT_Y_OFFSET - 2,
                             bounds.size.w - NEXT_TRAIN_CELL_TIME_X - CELL_MARGIN - CELL_ICON_SIZE - CELL_MARGIN,
                             CELL_HEIGHT_2);
    draw_text(ctx, str_time, FONT_KEY_GOTHIC_24_BOLD, frame_time, GTextAlignmentRight);
    
    // Terminus
    GRect frame_terminus = GRect(CELL_MARGIN,
                                 CELL_HEIGHT_2 + TEXT_Y_OFFSET + 1,
                                 bounds.size.w - CELL_MARGIN_2,
                                 CELL_HEIGHT_2);
    draw_text(ctx, str_terminus, FONT_KEY_GOTHIC_18, frame_terminus, GTextAlignmentLeft);
    
    // Platform
    GRect frame_platform = GRect(bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN,
                                 NEXT_TRAIN_CELL_ICON_Y,
                                 CELL_ICON_SIZE,
                                 CELL_ICON_SIZE);
    if (str_platform != NULL) {
#ifdef PBL_COLOR
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FRAME_DARK:RESOURCE_ID_IMG_FRAME_LIGHT, frame_platform);
#else
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_FRAME_LIGHT, frame_platform);
#endif
        draw_text(ctx, str_platform, FONT_KEY_GOTHIC_14_BOLD, frame_platform, GTextAlignmentCenter);
    } else {
        // TODO: No platform string
    }
}

// MARK: Data

static void set_from_to(StationIndex from, StationIndex to) {
    s_from_to.from = from;
    s_from_to.to = to;
    
    NULL_FREE(s_str_from);
    NULL_FREE(s_str_to);
    
    s_str_from = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    stations_get_name(s_from_to.from, s_str_from, STATION_NAME_MAX_LENGTH);
    if (s_from_to.to != STATION_NON) {
        s_str_to = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(s_from_to.to, s_str_to, STATION_NAME_MAX_LENGTH);
    }
    
    s_next_trains_list_count = 0;
}

static bool reverse_from_to() {
    if (s_from_to.to == STATION_NON) {
        return false;
    } else {
        set_from_to(s_from_to.to, s_from_to.from);
        return true;
    }
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
    return NEXT_TRAINS_ACTIONS_COUNT;
}

static size_t action_list_get_default_selection_callback(void) {
    return NEXT_TRAINS_ACTIONS_FAV;
}

static char* action_list_get_title_callback(size_t index) {
    switch (index) {
        case NEXT_TRAINS_ACTIONS_FAV:
            return "Set Favorite";
        default:
            return "";
    }
}

static bool action_list_is_enabled_callback(size_t index) {
    if (index == NEXT_TRAINS_ACTIONS_FAV) {
        return !fav_exists(s_from_to);
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    switch (index) {
        case NEXT_TRAINS_ACTIONS_FAV:
            fav_add(s_from_to.from, s_from_to.to);
            window_stack_remove(action_list_window, true);
            break;
        default:
            break;
    }
}

// MARK: Click Config Provider

static void long_select_handler_for_menu_layer(ClickRecognizerRef recognizer, void *context) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    if (selected_index.section == NEXT_TRAINS_SECTION_INFO) {
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

static void click_config_provider(void *context) {
    s_last_ccp(context);
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, long_select_handler_for_menu_layer, NULL);
}

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received) {
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (tuple_type->value->int8 != MESSAGE_TYPE_NEXT_TRAINS) {
        return;
    }
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    size_t count = tuple_payload_count->value->int16;
    
    s_next_trains_list_count = count;
    NULL_FREE(s_next_trains_list);
    if (s_next_trains_list_count > 0) {
        s_next_trains_list = malloc(sizeof(DataModelNextTrain) * s_next_trains_list_count);
    }
    
    for (uint32_t index = 0; index < count; ++index) {
        Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + index);
        if (tuple_payload->type == TUPLE_BYTE_ARRAY) {
            uint8_t *data = tuple_payload->value->data;
            uint16_t size_left = tuple_payload->length;
            size_t str_length = strlen((char *)data);
            size_t offset = str_length + 1;
            for (size_t data_index = 0; data_index < NEXT_TRAIN_KEY_COUNT && size_left > 0; ++data_index) {
                switch (data_index) {
                    case NEXT_TRAIN_KEY_CODE:
                        strncpy(s_next_trains_list[index].code, (char *)data, 5);
                        break;
                    case NEXT_TRAIN_KEY_HOUR:
                        if (size_left >= 4) {
                            s_next_trains_list[index].hour = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
                        }
                        break;
                    case NEXT_TRAIN_KEY_PLATFORM:
                        strncpy(s_next_trains_list[index].platform, (char *)data, 3);
                        break;
                    case NEXT_TRAIN_KEY_TERMINUS:
                        if (size_left >= 2) {
                            s_next_trains_list[index].terminus = (data[0] << 8) + data[1];
                        }
                        break;
                    case NEXT_TRAIN_KEY_NUMBER:
                    {
                        size_t number_length = (size_left >= TRAIN_NUMBER_LENGTH)?TRAIN_NUMBER_LENGTH-1:size_left;
                        strncpy(s_next_trains_list[index].number, (char *)data, number_length);
                        s_next_trains_list[index].number[number_length] = '\0';
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
    restart_timers();
    s_show_relative_time = false;
    menu_layer_reload_data(s_menu_layer);
}

static void message_failed_callback(void) {
    // TODO: Cancel loading...
}

static void request_next_stations() {
    // Update UI
    s_is_updating = true;
    menu_layer_reload_data(s_menu_layer);
    
    // Prepare parameters
    DictionaryIterator parameters;
    
    size_t tuple_count = 1;
    if (s_from_to.from != STATION_NON) {
        ++tuple_count;
    }
    if (s_from_to.to != STATION_NON) {
        ++tuple_count;
    }
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), STATION_CODE_LENGTH * (tuple_count - 1));
    uint8_t *dict_buffer = malloc(dict_size);
    dict_write_begin(&parameters, dict_buffer, dict_size);
    
    dict_write_uint8(&parameters, MESSAGE_KEY_REQUEST_TYPE, MESSAGE_TYPE_NEXT_TRAINS);
    
    if (s_from_to.from != STATION_NON) {
        char *data = malloc(STATION_CODE_LENGTH);
        stations_get_code(s_from_to.from, data, STATION_CODE_LENGTH);
        size_t length = strlen(data);
        if (length > STATION_CODE_LENGTH) {
            length = STATION_CODE_LENGTH;
        }
        dict_write_data(&parameters, MESSAGE_KEY_REQUEST_CODE_FROM, (uint8_t *)data, length);
        free(data);
    }
    if (s_from_to.to != STATION_NON) {
        char *data = malloc(STATION_CODE_LENGTH);
        stations_get_code(s_from_to.to, data, STATION_CODE_LENGTH);
        size_t length = strlen(data);
        if (length > STATION_CODE_LENGTH) {
            length = STATION_CODE_LENGTH;
        }
        dict_write_data(&parameters, MESSAGE_KEY_REQUEST_CODE_TO, (uint8_t *)data, length);
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

static void reload_data_timer_callback(void *context);

static void reload_data_timer_start() {
    s_reload_data_timer = app_timer_register(RELOAD_DATA_TIMER_INTERVAL, reload_data_timer_callback, NULL);
}

static void reload_data_timer_stop() {
    if(s_reload_data_timer) {
        app_timer_cancel(s_reload_data_timer);
        s_reload_data_timer = NULL;
    }
}

static void reload_data_timer_callback(void *context) {
    request_next_stations();
    reload_data_timer_start();
}

static void restart_timers() {
    update_time_format_timer_stop();
    update_time_format_timer_start();
    reload_data_timer_stop();
    reload_data_timer_start();
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return NEXT_TRAINS_SECTION_COUNT;
}

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == NEXT_TRAINS_SECTION_INFO) {
        return 1;
    } else if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        if (s_is_updating && s_next_trains_list_count == 0) {
            return 1;
        } else {
            return (s_next_trains_list_count > 0)?s_next_trains_list_count:1;
        }
    }
    return 0;
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT;
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
    
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        draw_from_to(ctx,
                     cell_layer,
                     s_from_to,
#ifdef PBL_COLOR
                     is_highlighed,
#endif
                     text_color);
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (s_is_updating && s_next_trains_list_count == 0) {
            draw_centered_title(ctx, cell_layer, "Loading...", NULL,
#ifdef PBL_COLOR
                                GColorLightGray
#else
                                text_color
#endif
                                );
        } else if (s_next_trains_list_count > 0) {
            DataModelNextTrain next_train = s_next_trains_list[cell_index->row];
            
            // Hour
            char *str_hour = calloc(TIME_STRING_LENGTH, sizeof(char));
            time_2_str(next_train.hour, str_hour, TIME_STRING_LENGTH, s_show_relative_time);
            
            // Terminus
            char *str_terminus = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
            stations_get_name(next_train.terminus, str_terminus, STATION_NAME_MAX_LENGTH);
            
            draw_menu_layer_cell(ctx,
                                 cell_layer,
                                 text_color,
#ifdef PBL_COLOR
                                 is_highlighed,
#endif
                                 next_train.code,
                                 str_hour,
                                 str_terminus,
                                 next_train.platform);
            
            // Clean
            free(str_hour);
            free(str_terminus);
        } else {
            draw_centered_title(ctx, cell_layer, "No train.", NULL,
#ifdef PBL_COLOR
                                GColorLightGray
#else
                                text_color
#endif
                                );
        }
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        if (reverse_from_to()) {
            request_next_stations();
        }
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (!s_is_updating && s_next_trains_list_count > 0) {
            DataModelNextTrain next_train = s_next_trains_list[cell_index->row];
            push_train_details_window(next_train.number, s_from_to.from, true);
        }
    }
}

#ifdef PBL_PLATFORM_BASALT

static int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 1;
}

static void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)  {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

static void menu_layer_selection_will_change_callback(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, void *callback_context) {
    if (new_index->section == NEXT_TRAINS_SECTION_TRAINS && s_next_trains_list_count == 0) {
        *new_index = old_index;
    }
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
#ifdef PBL_PLATFORM_BASALT
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer);
#endif
    
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
        .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)menu_layer_get_num_sections_callback,
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback
#ifdef PBL_PLATFORM_BASALT
        ,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)menu_layer_draw_separator_callback,
        .selection_will_change = (MenuLayerSelectionWillChangeCallback)menu_layer_selection_will_change_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)menu_layer_draw_background_callback
#endif
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    s_last_ccp = window_get_click_config_provider(window);
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
    NULL_FREE(s_str_from);
    NULL_FREE(s_str_to);
    NULL_FREE(s_next_trains_list);
    
    // Window
    menu_layer_destroy(s_menu_layer);
    window_destroy(s_window);
    s_window = NULL;
    
#ifdef PBL_PLATFORM_BASALT
    layer_destroy(s_status_bar_background_layer);
    status_bar_layer_destroy(s_status_bar);
#endif
    
#ifdef PBL_BW
    inverter_layer_destroy(s_inverter_layer);
#endif
}

static void window_appear(Window *window) {
    if (s_next_trains_list == NULL) {
        request_next_stations();
    }
    
    // Start timer
    reload_data_timer_start();
    update_time_format_timer_start();
}

static void window_disappear(Window *window) {
    message_clear_callbacks();
    
    // Stop timer
    reload_data_timer_stop();
    update_time_format_timer_stop();
}

// MARK: Entry point

void push_next_trains_window(DataModelFromTo from_to, bool animated) {
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
    }
    
    // Reset data
    set_from_to(from_to.from, from_to.to);
    
    s_next_trains_list_count = 0;
    s_is_updating = false;
    
    // Reset some status
    NULL_FREE(s_next_trains_list);
    s_show_relative_time = false;
    
    // Push window
    window_stack_push(s_window, animated);
}
