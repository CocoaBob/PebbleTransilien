//
//  window_next_trains.c
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
    
    DataModelFromTo from_to;
    char *str_from;
    char *str_to;
    
    size_t next_trains_list_count;
    DataModelNextTrain *next_trains_list;
    bool is_updating;
    
    bool show_relative_time;
} NextTrains;

// Forward declaration

#if !defined(PBL_PLATFORM_APLITE)
static void restart_timers(NextTrains *user_info);
#endif

// MARK: Constants

#define NEXT_TRAIN_CELL_ICON_Y 4                    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_SUB_ICON_Y 27               // CELL_HEIGHT_2 + 5 = 22 + 5
#define NEXT_TRAIN_CELL_SUB_ICON_RIGHT_MARGIN 7     // CELL_MARGIN + (CELL_ICON_SIZE - CELL_SUB_ICON_SIZE) / 2 = 4 + (19 - 13) / 2 = 7
#define NEXT_TRAIN_CELL_MENTION_Y_OFFSET 4
#define NEXT_TRAIN_CELL_MENTION_RIGHT_MARGIN 2
#define NEXT_TRAIN_CELL_CODE_X 4                    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_CODE_W 56
#define NEXT_TRAIN_CELL_TIME_X 64                   // CELL_MARGIN + NEXT_TRAIN_CELL_CODE_W + CELL_MARGIN = 4 + 56 + 4

// MARK: Drawing

static void draw_menu_layer_cell(GContext *ctx,
                                 Layer *cell_layer,
#ifdef PBL_COLOR
                                 GColor text_color,
                                 bool is_highlighed,
#endif
                                 bool is_selected,
                                 const char * str_code,
                                 const char * str_time,
                                 const char * str_terminus,
                                 const char * str_platform,
                                 const char * str_mention) {
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, text_color);
#else
    graphics_context_set_text_color(ctx, GColorBlack);
#endif
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
    
    // Platform
    GRect frame_platform = GRect(bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN,
                                 NEXT_TRAIN_CELL_ICON_Y,
                                 CELL_ICON_SIZE,
                                 CELL_ICON_SIZE);
    if (str_platform != NULL) {
#ifdef PBL_COLOR
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FRAME_DARK:RESOURCE_ID_IMG_FRAME_LIGHT, frame_platform);
#else
        draw_image_in_rect(ctx, false, RESOURCE_ID_IMG_FRAME_LIGHT, frame_platform);
#endif
        draw_text(ctx, str_platform, FONT_KEY_GOTHIC_14_BOLD, frame_platform, GTextAlignmentCenter);
    } else {
        // TODO: No platform string
    }
    
    // Mention
    bool has_mention = (str_mention != NULL && strlen(str_mention) > 0);
    
    // Terminus
    GRect frame_terminus = GRect(CELL_MARGIN,
                                 CELL_HEIGHT_2 + TEXT_Y_OFFSET + 1,
                                 bounds.size.w - CELL_MARGIN_2,
                                 CELL_HEIGHT_2);
    if (has_mention) {
        if (is_selected) {
            // Draw mention text
            GRect frame_mention = GRect(CELL_MARGIN,
                                        CELL_HEIGHT_2 + TEXT_Y_OFFSET + NEXT_TRAIN_CELL_MENTION_Y_OFFSET,
                                        bounds.size.w - CELL_MARGIN_2 - NEXT_TRAIN_CELL_MENTION_RIGHT_MARGIN,
                                        14);
            GSize mention_size = graphics_text_layout_get_content_size(str_mention,
                                                                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                                                                       frame_mention,
                                                                       GTextOverflowModeTrailingEllipsis,
                                                                       GTextAlignmentRight);
            if (mention_size.w > 0) {
                frame_mention.size.w = mention_size.w;
                frame_terminus.size.w -= CELL_MARGIN + mention_size.w + NEXT_TRAIN_CELL_MENTION_RIGHT_MARGIN;
                
                frame_mention.origin.x = frame_terminus.origin.x + frame_terminus.size.w + CELL_MARGIN;
                draw_text(ctx,
                          str_mention,
                          FONT_KEY_GOTHIC_14,
                          frame_mention,
                          GTextAlignmentRight);
                
                // Draw a mention frame
                frame_mention.origin.x -= 2;
                frame_mention.size.w += 4;
                frame_mention.origin.y = NEXT_TRAIN_CELL_SUB_ICON_Y - 1;
                frame_mention.size.h += 1;
#ifdef PBL_COLOR
                graphics_context_set_stroke_color(ctx, text_color);
#else
                graphics_context_set_stroke_color(ctx, GColorBlack);
#endif
                graphics_draw_round_rect(ctx, frame_mention, 2);
            }
            
        } else {
            frame_terminus.size.w -= CELL_MARGIN + CELL_SUB_ICON_SIZE + NEXT_TRAIN_CELL_SUB_ICON_RIGHT_MARGIN;
            
            // Draw warning icon
            GRect frame_mention = GRect(bounds.size.w - CELL_SUB_ICON_SIZE - NEXT_TRAIN_CELL_SUB_ICON_RIGHT_MARGIN,
                                        NEXT_TRAIN_CELL_SUB_ICON_Y,
                                        CELL_SUB_ICON_SIZE,
                                        CELL_SUB_ICON_SIZE);
            
#ifdef PBL_COLOR
            draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_MENTION_DARK:RESOURCE_ID_IMG_MENTION_LIGHT, frame_mention);
#else
            draw_image_in_rect(ctx, false, RESOURCE_ID_IMG_MENTION_LIGHT, frame_mention);
#endif
        }
    }
    draw_text(ctx,
              str_terminus,
              FONT_KEY_GOTHIC_18,
              frame_terminus,
              GTextAlignmentLeft);
    
}

// MARK: Data

static void release_next_trains_list(NextTrains *user_info) {
    for (size_t idx = 0; idx < user_info->next_trains_list_count; ++idx) {
        NULL_FREE(user_info->next_trains_list[idx].code);
        NULL_FREE(user_info->next_trains_list[idx].platform);
        NULL_FREE(user_info->next_trains_list[idx].number);
        NULL_FREE(user_info->next_trains_list[idx].mention);
    }
    NULL_FREE(user_info->next_trains_list);
    user_info->next_trains_list_count = 0;
}

static void set_from_to(StationIndex from, StationIndex to, NextTrains *user_info) {
    user_info->from_to.from = from;
    user_info->from_to.to = to;
    
    NULL_FREE(user_info->str_from);
    NULL_FREE(user_info->str_to);
    
    user_info->str_from = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    stations_get_name(user_info->from_to.from, user_info->str_from, STATION_NAME_MAX_LENGTH);
    if (user_info->from_to.to != STATION_NON) {
        user_info->str_to = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(user_info->from_to.to, user_info->str_to, STATION_NAME_MAX_LENGTH);
    }
    
    release_next_trains_list(user_info);
}

static bool reverse_from_to(NextTrains *user_info) {
    if (user_info->from_to.to == STATION_NON) {
        return false;
    } else {
        set_from_to(user_info->from_to.to, user_info->from_to.from, user_info);
        return true;
    }
}

// MARK: Action list callbacks

static char* action_list_get_title_callback(size_t index, NextTrains *user_info) {
    return _("Set Favorite");
}

static bool action_list_is_enabled_callback(size_t index, NextTrains *user_info) {
    if (index == NEXT_TRAINS_ACTIONS_FAV) {
        return !fav_exists(user_info->from_to);
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index, NextTrains *user_info) {
    switch (index) {
        case NEXT_TRAINS_ACTIONS_FAV:
            fav_add(user_info->from_to.from, user_info->from_to.to);
            window_stack_remove(action_list_window, true);
            break;
    }
}

// MARK: Click Config Provider

static void window_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    MenuLayer *menu_layer = context;
    MenuIndex selected_index = menu_layer_get_selected_index(menu_layer);
    if (selected_index.section == NEXT_TRAINS_SECTION_INFO) {
        ActionListConfig config = (ActionListConfig){
            .context = menu_layer,
            .num_rows = NEXT_TRAINS_ACTIONS_COUNT,
            .default_selection = NEXT_TRAINS_ACTIONS_FAV,
#ifdef PBL_COLOR
            .colors = {
                .background = GColorCobaltBlue,
                .foreground = GColorBlack,
                .text = GColorLightGray,
                .text_selected = GColorWhite,
                .text_disabled = GColorDarkGray,
            },
#endif
            .callbacks = {
                .get_title = (ActionListGetTitleCallback)action_list_get_title_callback,
                .is_enabled = (ActionListIsEnabledCallback)action_list_is_enabled_callback,
                .select_click = (ActionListSelectCallback)action_list_select_callback
            }
        };
        action_list_open(&config);
    }
}

static void click_config_provider(void *context) {
    MenuLayer *menu_layer = context;
    NextTrains *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
    
    user_info->last_ccp(user_info->menu_layer);
    
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, window_long_click_handler, NULL);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, menu_layer_button_up_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30, menu_layer_button_down_handler);
}

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received, NextTrains *user_info) {
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (tuple_type->value->int8 != MESSAGE_TYPE_NEXT_TRAINS) {
        return;
    }
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    size_t count = tuple_payload_count->value->int16;
    
    release_next_trains_list(user_info);
    user_info->next_trains_list_count = count;
    if (user_info->next_trains_list_count > 0) {
        user_info->next_trains_list = malloc(sizeof(DataModelNextTrain) * user_info->next_trains_list_count);
    }
    
    for (size_t idx = 0; idx < count; ++idx) {
        Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + idx);
        if (tuple_payload->type == TUPLE_BYTE_ARRAY) {
            uint8_t *data = tuple_payload->value->data;
            uint16_t size_left = tuple_payload->length;
            size_t str_length = 0,offset = 0;
            for (size_t data_index = 0; data_index < NEXT_TRAIN_KEY_COUNT && size_left > 0; ++data_index) {
                data += offset;
                str_length = (data_index == NEXT_TRAIN_KEY_HOUR)?4:strlen((char *)data);
                offset = str_length + 1;
                
                // Interger data
                if (data_index == NEXT_TRAIN_KEY_HOUR ||
                    data_index == NEXT_TRAIN_KEY_TERMINUS) {
                    long long temp_int = 0;
                    for (size_t i = 0; i < str_length; ++i) {
                        temp_int += data[i] << (8 * (str_length - i - 1));
                    }
                    if (data_index == NEXT_TRAIN_KEY_HOUR) {
                        user_info->next_trains_list[idx].hour = temp_int;
                    } else if (data_index == NEXT_TRAIN_KEY_TERMINUS) {
                        user_info->next_trains_list[idx].terminus = temp_int;
                    }
                }
                // C string data
                else {
                    char *string = calloc(offset, sizeof(char));
                    strncpy(string, (char *)data, offset);
                    if (data_index == NEXT_TRAIN_KEY_CODE) {
                        user_info->next_trains_list[idx].code = string;
                    } else if (data_index == NEXT_TRAIN_KEY_PLATFORM) {
                        user_info->next_trains_list[idx].platform = string;
                    } else if (data_index == NEXT_TRAIN_KEY_NUMBER) {
                        user_info->next_trains_list[idx].number = string;
                    } else if (data_index == NEXT_TRAIN_KEY_MENTION) {
                        user_info->next_trains_list[idx].mention = string;
                    }
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
    vibes_short_pulse();
}

static void message_failed_callback(NextTrains *user_info) {
    // TODO: Cancel loading...
}

static void write_request_dict(DictionaryIterator *parameters, StationIndex station_index, MESSAGE_KEY message_key) {
    if (station_index != STATION_NON) {
        char *data = malloc(STATION_CODE_LENGTH);
        stations_get_code(station_index, data, STATION_CODE_LENGTH);
        size_t length = strlen(data);
        if (length > STATION_CODE_LENGTH) {
            length = STATION_CODE_LENGTH;
        }
        dict_write_data(parameters, message_key, (uint8_t *)data, length);
        free(data);
    }
}

static void request_next_stations(NextTrains *user_info) {
    // Update UI
    user_info->is_updating = true;
    menu_layer_reload_data(user_info->menu_layer);
    
    // Prepare parameters
    DictionaryIterator parameters;
    
    size_t tuple_count = 1;
    if (user_info->from_to.from != STATION_NON) {
        ++tuple_count;
    }
    if (user_info->from_to.to != STATION_NON) {
        ++tuple_count;
    }
    uint32_t dict_size = dict_calc_buffer_size(tuple_count, sizeof(uint8_t), STATION_CODE_LENGTH * (tuple_count - 1));
    uint8_t *dict_buffer = malloc(dict_size);
    
    dict_write_begin(&parameters, dict_buffer, dict_size);
    dict_write_uint8(&parameters, MESSAGE_KEY_REQUEST_TYPE, MESSAGE_TYPE_NEXT_TRAINS);
    write_request_dict(&parameters, user_info->from_to.from, MESSAGE_KEY_REQUEST_CODE_FROM);
    write_request_dict(&parameters, user_info->from_to.to, MESSAGE_KEY_REQUEST_CODE_TO);
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

static void format_timer_callback(NextTrains *user_info);

static void format_timer_start(NextTrains *user_info) {
    user_info->format_timer = app_timer_register(UPDATE_TIME_FORMAT_INTERVAL, (AppTimerCallback)format_timer_callback, user_info);
}

static void format_timer_stop(NextTrains *user_info) {
    if(user_info->format_timer) {
        app_timer_cancel(user_info->format_timer);
    }
}

static void format_timer_callback(NextTrains *user_info) {
    user_info->show_relative_time = !user_info->show_relative_time;
    menu_layer_reload_data(user_info->menu_layer);
    format_timer_start(user_info);
}

static void restart_timers(NextTrains *user_info) {
    format_timer_stop(user_info);
    format_timer_start(user_info);
}

#endif

// MARK: Accel Tap Service

static void accel_tap_service_handler(AccelAxisType axis, int32_t direction, void *context) {
    request_next_stations(context);
}

// MARK: Tick Timer Service

static void tick_timer_service_handler(struct tm *tick_time, TimeUnits units_changed, NextTrains *user_info) {
    status_bar_update();
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_sections_callback(struct MenuLayer *menu_layer, NextTrains *user_info) {
    return NEXT_TRAINS_SECTION_COUNT;
}

static uint16_t menu_layer_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, NextTrains *user_info) {
    if (section_index == NEXT_TRAINS_SECTION_INFO) {
        return 1;
    } else if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        if (user_info->is_updating && user_info->next_trains_list_count == 0) {
            return 1;
        } else {
            return (user_info->next_trains_list_count > 0)?user_info->next_trains_list_count:1;
        }
    }
    return 0;
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, NextTrains *user_info) {
    return CELL_HEIGHT;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, NextTrains *user_info) {
    MenuIndex selected_index = menu_layer_get_selected_index(user_info->menu_layer);
    bool is_selected = (menu_index_compare(&selected_index, cell_index) == 0);
#ifdef PBL_COLOR
    bool is_highlighed = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#endif
    
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        draw_from_to(ctx, cell_layer,
#ifdef PBL_COLOR
                     is_highlighed,
                     text_color,
#else
                     false,
#endif
                     user_info->from_to);
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (user_info->is_updating && user_info->next_trains_list_count == 0) {
            draw_centered_title(ctx, cell_layer,
                                is_selected,
                                _("Loading..."),
                                NULL);
        } else if (user_info->next_trains_list_count > 0) {
            DataModelNextTrain next_train = user_info->next_trains_list[cell_index->row];
            
            // Hour
            char *str_hour = calloc(TIME_STRING_LENGTH, sizeof(char));
            time_2_str(next_train.hour, str_hour, TIME_STRING_LENGTH, user_info->show_relative_time);
            
            // Terminus
            char *str_terminus = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
            stations_get_name(next_train.terminus, str_terminus, STATION_NAME_MAX_LENGTH);
            
            draw_menu_layer_cell(ctx, cell_layer,
#ifdef PBL_COLOR
                                 text_color,
                                 is_highlighed,
#endif
                                 is_selected,
                                 next_train.code,
                                 str_hour,
                                 str_terminus,
                                 next_train.platform,
                                 next_train.mention);
            
            // Clean
            free(str_hour);
            free(str_terminus);
        } else {
            draw_centered_title(ctx, cell_layer,
                                is_selected,
                                _("No train."),
                                NULL);
        }
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, NextTrains *user_info) {
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        if (reverse_from_to(user_info)) {
            request_next_stations(user_info);
        }
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (!user_info->is_updating && user_info->next_trains_list_count > 0) {
            DataModelNextTrain next_train = user_info->next_trains_list[cell_index->row];
            push_window_train_details(next_train.number, user_info->from_to.from, true);
        }
    }
}

#if !defined(PBL_PLATFORM_APLITE)

static void menu_layer_selection_will_change_callback(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, NextTrains *user_info) {
    if (new_index->section == NEXT_TRAINS_SECTION_TRAINS && user_info->next_trains_list_count == 0) {
        *new_index = old_index;
    }
}

#endif

// MARK: Window callbacks

static void window_load(Window *window) {
    NextTrains *user_info = window_get_user_data(window);
    
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
    menu_layer_set_callbacks(user_info->menu_layer, user_info, (MenuLayerCallbacks) {
        .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)menu_layer_get_num_sections_callback,
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback
#if !defined(PBL_PLATFORM_APLITE)
        ,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)menu_layer_draw_separator_callback,
        .selection_will_change = (MenuLayerSelectionWillChangeCallback)menu_layer_selection_will_change_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)menu_layer_draw_background_callback
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
    NextTrains *user_info = window_get_user_data(window);
    
    // Data
    NULL_FREE(user_info->str_from);
    NULL_FREE(user_info->str_to);
    release_next_trains_list(user_info);
    
    // Window
    menu_layer_destroy(user_info->menu_layer);
#ifdef PBL_BW
    inverter_layer_destroy(user_info->inverter_layer);
#endif
    window_destroy(user_info->window);
    
    NULL_FREE(user_info);
}

static void window_appear(Window *window) {
    // Discard formerly sent requests
    message_clear_callbacks();
    
    NextTrains *user_info = window_get_user_data(window);
    
    // Add status bar
    ui_setup_status_bar(window_get_root_layer(user_info->window), menu_layer_get_layer(user_info->menu_layer));
    
    // Subscribe services
    accel_tap_service_init(accel_tap_service_handler, user_info);
    tick_timer_service_init((TickTimerServiceHandler)tick_timer_service_handler, user_info);
    
    // Request data
    if (user_info->next_trains_list == NULL) {
        request_next_stations(user_info);
    }
    
    // Start UI timer
#if !defined(PBL_PLATFORM_APLITE)
    format_timer_start(user_info);
#endif
    
//    printf("Heap Total <%4dB> Used <%4dB> Free <%4dB>",heap_bytes_used()+heap_bytes_free(),heap_bytes_used(),heap_bytes_free());
}

static void window_disappear(Window *window) {
    // Unsubscribe services
    accel_tap_service_deinit();
    tick_timer_service_deinit();
    
    // Stop UI timer
#if !defined(PBL_PLATFORM_APLITE)
    NextTrains *user_info = window_get_user_data(window);
    format_timer_stop(user_info);
#endif
}

// MARK: Entry point

void push_window_next_trains(DataModelFromTo from_to, bool animated) {
    NextTrains *user_info = calloc(1, sizeof(NextTrains));
    if (user_info) {
        user_info->window = window_create();
        window_set_user_data(user_info->window, user_info);
        window_set_window_handlers(user_info->window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
        
        // Reset data
        set_from_to(from_to.from, from_to.to, user_info);
        user_info->is_updating = false;
        
        // Reset some status
        user_info->show_relative_time = false;
        
#ifdef PBL_SDK_2
        // Fullscreen
        window_set_fullscreen(user_info->window, true);
#endif
        
        // Push window
        window_stack_push(user_info->window, animated);
    }
}
