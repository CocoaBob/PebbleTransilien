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

static Window *s_window;
static MenuLayer *s_menu_layer;
static ClickConfigProvider s_last_ccp;
#if !defined(PBL_PLATFORM_APLITE)
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

#if !defined(PBL_PLATFORM_APLITE)
#define UPDATE_TIME_FORMAT_INTERVAL 3750 // 3.75 seconds
static AppTimer *s_update_time_format_timer;
#define RELOAD_DATA_TIMER_INTERVAL 15000 // 15 seconds
static AppTimer *s_reload_data_timer;
#define IDLE_TIMEOUT 300000 // 5 minutes
static AppTimer *s_idle_timer;
#endif

static DataModelFromTo s_from_to;
static char *s_str_from;
static char *s_str_to;

static size_t s_next_trains_list_count;
static DataModelNextTrain *s_next_trains_list;
static bool s_is_updating;

static bool s_show_relative_time;

// Forward declaration

#if !defined(PBL_PLATFORM_APLITE)
static void restart_timers();
static void idle_timer_start();
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

static void release_next_trains_list() {
    for (size_t idx = 0; idx < s_next_trains_list_count; ++idx) {
        NULL_FREE(s_next_trains_list[idx].code);
        NULL_FREE(s_next_trains_list[idx].platform);
        NULL_FREE(s_next_trains_list[idx].number);
        NULL_FREE(s_next_trains_list[idx].mention);
    }
    NULL_FREE(s_next_trains_list);
    s_next_trains_list_count = 0;
}

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
    
    release_next_trains_list();
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

static char* action_list_get_title_callback(size_t index) {
    return _("Set Favorite");
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
    }
}

// MARK: Click Config Provider

static void window_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex selected_index = menu_layer_get_selected_index(context);
    if (selected_index.section == NEXT_TRAINS_SECTION_INFO) {
        ActionListConfig config = (ActionListConfig){
//            .context = main_menu,
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
    s_last_ccp(context);
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, window_long_click_handler, NULL);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, menu_layer_button_up_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30, menu_layer_button_down_handler);
}

// MARK: Message Request callbacks

static void message_succeeded_callback(DictionaryIterator *received) {
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (tuple_type->value->int8 != MESSAGE_TYPE_NEXT_TRAINS) {
        return;
    }
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    size_t count = tuple_payload_count->value->int16;
    
    release_next_trains_list();
    s_next_trains_list_count = count;
    if (s_next_trains_list_count > 0) {
        s_next_trains_list = malloc(sizeof(DataModelNextTrain) * s_next_trains_list_count);
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
                        s_next_trains_list[idx].hour = temp_int;
                    } else if (data_index == NEXT_TRAIN_KEY_TERMINUS) {
                        s_next_trains_list[idx].terminus = temp_int;
                    }
                }
                // C string data
                else {
                    char *temp_string = calloc(offset, sizeof(char));
                    strncpy(temp_string, (char *)data, offset);
                    if (data_index == NEXT_TRAIN_KEY_CODE) {
                        s_next_trains_list[idx].code = temp_string;
                    } else if (data_index == NEXT_TRAIN_KEY_PLATFORM) {
                        s_next_trains_list[idx].platform = temp_string;
                    } else if (data_index == NEXT_TRAIN_KEY_NUMBER) {
                        s_next_trains_list[idx].number = temp_string;
                    } else if (data_index == NEXT_TRAIN_KEY_MENTION) {
                        s_next_trains_list[idx].mention = temp_string;
                    }
                }
                
                size_left -= (uint16_t)offset;
            }
        }
    }
    
    // Update UI
    s_is_updating = false;
#if !defined(PBL_PLATFORM_APLITE)
    restart_timers();
#endif
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

#if !defined(PBL_PLATFORM_APLITE)
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

static void idle_timer_callback(void *context) {
    window_stack_pop(true); // Pop to Main window
}

static void idle_timer_start() {
    s_idle_timer = app_timer_register(IDLE_TIMEOUT, idle_timer_callback, NULL);
}

static void idle_timer_stop() {
    if(s_idle_timer) {
        app_timer_cancel(s_idle_timer);
        s_idle_timer = NULL;
    }
}
#endif

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
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
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
                     s_from_to);
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (s_is_updating && s_next_trains_list_count == 0) {
            draw_centered_title(ctx, cell_layer,
                                _("Loading..."),
                                NULL);
        } else if (s_next_trains_list_count > 0) {
            DataModelNextTrain next_train = s_next_trains_list[cell_index->row];
            
            // Hour
            char *str_hour = calloc(TIME_STRING_LENGTH, sizeof(char));
            time_2_str(next_train.hour, str_hour, TIME_STRING_LENGTH, s_show_relative_time);
            
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
                                _("No train."),
                                NULL);
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
            push_window_train_details(next_train.number, s_from_to.from, true);
        }
    }
}

#if !defined(PBL_PLATFORM_APLITE)

static void menu_layer_selection_will_change_callback(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, void *callback_context) {
    if (new_index->section == NEXT_TRAINS_SECTION_TRAINS && s_next_trains_list_count == 0) {
        *new_index = old_index;
    }
}

#endif

// MARK: Window callbacks

static void window_load(Window *window) {
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add status bar
#if !defined(PBL_PLATFORM_APLITE)
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer);
#endif
    
    // Add menu layer
    int16_t status_bar_height = 0;
#if !defined(PBL_PLATFORM_APLITE)
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
#if !defined(PBL_PLATFORM_APLITE)
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
    ui_setup_theme(s_window, s_menu_layer);
#else
    ui_setup_theme(s_window, s_inverter_layer);
#endif
}

static void window_unload(Window *window) {
    // Data
    NULL_FREE(s_str_from);
    NULL_FREE(s_str_to);
    release_next_trains_list();
    
    // Window
    menu_layer_destroy(s_menu_layer);
    window_destroy(s_window);
    s_window = NULL;
    
#if !defined(PBL_PLATFORM_APLITE)
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
#if !defined(PBL_PLATFORM_APLITE)
    update_time_format_timer_start();
    reload_data_timer_start();
    idle_timer_start();
#endif
//    printf("Heap Total <%4dB> Used <%4dB> Free <%4dB>",heap_bytes_used()+heap_bytes_free(),heap_bytes_used(),heap_bytes_free());
}

static void window_disappear(Window *window) {
    message_clear_callbacks();
    
#if !defined(PBL_PLATFORM_APLITE)
    // Stop timer
    update_time_format_timer_stop();
    reload_data_timer_stop();
    idle_timer_stop();
#endif
}

// MARK: Entry point

void push_window_next_trains(DataModelFromTo from_to, bool animated) {
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
    s_is_updating = false;
    
    // Reset some status
    s_show_relative_time = false;
    
    // Push window
    window_stack_push(s_window, animated);
}
