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
    NEXT_TRAINS_SECTION_INFO,
    NEXT_TRAINS_SECTION_TRAINS,
    NEXT_TRAINS_SECTION_COUNT
};

enum {
    NEXT_TRAINS_ACTIONS_EDIT,
#if EXTRA_INFO_IS_ENABLED
    NEXT_TRAINS_ACTIONS_EXTRA_INFO,
#endif
    NEXT_TRAINS_ACTIONS_FAV,
    NEXT_TRAINS_ACTIONS_COUNT
};

typedef struct {
    Window *window;
    MenuLayer *menu_layer;
    
    ClickConfigProvider last_ccp;
    
#if RELATIVE_TIME_IS_ENABLED
#define UPDATE_TIME_FORMAT_INTERVAL 3000 // 3 seconds
    AppTimer *format_timer;
    bool show_relative_time;
#endif
    
    DataModelFromTo from_to;
    char *str_from;
    char *str_to;
    
    int next_trains_list_count;
    DataModelNextTrain *next_trains_list;
    
#if EXTRA_INFO_IS_ENABLED
    char *extra_info;
#endif
    
    bool is_updating;
    
#if TEXT_SCROLL_IS_ENABLED
    TextScrollData *text_scroll_data;
#endif
} NextTrains;

// Forward declaration

#if RELATIVE_TIME_IS_ENABLED
static void restart_timers(NextTrains *user_info);
#endif

// MARK: Constants

#define NEXT_TRAIN_CELL_ICON_Y 4                    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_SUB_ICON_Y 27               // CELL_HEIGHT_2 + 5 = 22 + 5
#define NEXT_TRAIN_CELL_SUB_ICON_RIGHT_MARGIN 7     // CELL_MARGIN + (CELL_ICON_SIZE - CELL_SUB_ICON_SIZE) / 2 = 4 + (19 - 13) / 2 = 7
#define NEXT_TRAIN_CELL_MENTION_Y_OFFSET 4
#define NEXT_TRAIN_CELL_MENTION_RIGHT_MARGIN 2
#define NEXT_TRAIN_CELL_CODE_X 4                    // CELL_MARGIN = 4
#define NEXT_TRAIN_CELL_TIME_W 53

// MARK: Text Scroll callback
#if TEXT_SCROLL_IS_ENABLED
static void text_scroll_update(TextScrollConfig *config) {
    if (config->user_context != NULL) {
        layer_mark_dirty(config->user_context);
    }
}
#endif

// MARK: Drawing

static void draw_menu_layer_cell(GContext *ctx,
                                 Layer *cell_layer,
                                 GColor text_color,
                                 bool is_inverted,
                                 bool is_selected,
                                 char * str_code,
                                 char * str_time,
                                 char * str_terminus,
                                 char * str_platform,
                                 char * str_mention,
                                 NextTrains *user_info) {
    graphics_context_set_text_color(ctx, text_color);;
    GRect bounds = layer_get_bounds(cell_layer);
    
    // Code
    GRect frame_code = GRect(NEXT_TRAIN_CELL_CODE_X,
                             TEXT_Y_OFFSET - 2,
                             bounds.size.w - NEXT_TRAIN_CELL_CODE_X - CELL_MARGIN - NEXT_TRAIN_CELL_TIME_W - CELL_MARGIN - CELL_ICON_SIZE - CELL_MARGIN,
                             CELL_HEIGHT_2);
    
    // Time
    GRect frame_time = GRect(bounds.size.w - NEXT_TRAIN_CELL_TIME_W - CELL_MARGIN - CELL_ICON_SIZE - CELL_MARGIN,
                             TEXT_Y_OFFSET - 2,
                             NEXT_TRAIN_CELL_TIME_W,
                             CELL_HEIGHT_2);
    
    // Platform
    GRect frame_platform = GRect(bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN,
                                 NEXT_TRAIN_CELL_ICON_Y,
                                 CELL_ICON_SIZE,
                                 CELL_ICON_SIZE);
    
    // Terminus
    GRect frame_terminus = GRect(CELL_MARGIN,
                                 CELL_HEIGHT_2 + TEXT_Y_OFFSET + 1,
                                 bounds.size.w - CELL_MARGIN_2,
                                 CELL_HEIGHT_2);
    
#ifdef PBL_ROUND
    GPoint line_1_offset = GPoint(0, layer_convert_point_to_screen(cell_layer, GPoint(0, frame_code.origin.y + CELL_HEIGHT_4)).y);
    line_1_offset.x = get_round_border_x_radius_82(line_1_offset.y - CELL_MARGIN_2);
    
    GPoint line_2_offset = GPoint(0, layer_convert_point_to_screen(cell_layer, GPoint(0, frame_terminus.origin.y + CELL_HEIGHT_4)).y);
    line_2_offset.x = get_round_border_x_radius_82(line_2_offset.y - CELL_MARGIN_2);
    
    frame_code.origin.x = line_1_offset.x + CELL_MARGIN_2;
    frame_code.size.w -= line_1_offset.x;
    
    frame_time.origin.x -= line_1_offset.x + CELL_MARGIN;
    frame_platform.origin.x -= line_1_offset.x + CELL_MARGIN;
    
    frame_terminus.origin.x = line_2_offset.x + CELL_MARGIN_2;
    frame_terminus.size.w -= line_2_offset.x + line_2_offset.x + CELL_MARGIN_4;
#endif
    
    // Draw Time/Platform/Terminus
    draw_text(ctx, str_code, FONT_KEY_GOTHIC_24_BOLD, frame_code, GTextAlignmentLeft);
    draw_text(ctx, str_time, FONT_KEY_GOTHIC_24_BOLD, frame_time, GTextAlignmentRight);
    if (str_platform != NULL) {
        graphics_context_set_stroke_color(ctx, is_inverted?GColorWhite:GColorBlack);
        graphics_draw_round_rect(ctx, frame_platform, 2);
        graphics_draw_rect(ctx, grect_crop(frame_platform, 1));
        draw_text(ctx, str_platform, FONT_KEY_GOTHIC_14_BOLD, frame_platform, GTextAlignmentCenter);
    }
    
    // Mention
    bool has_mention = (str_mention != NULL && strlen(str_mention) > 0);
    if (has_mention) {
        if (is_selected) {
            // Draw mention text
            GRect frame_mention = GRect(CELL_MARGIN,
                                        CELL_HEIGHT_2 + TEXT_Y_OFFSET + NEXT_TRAIN_CELL_MENTION_Y_OFFSET,
                                        bounds.size.w - CELL_MARGIN_2 - NEXT_TRAIN_CELL_MENTION_RIGHT_MARGIN,
                                        14);
            GSize mention_size = size_of_text(str_mention, FONT_KEY_GOTHIC_14, frame_mention);
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
                graphics_context_set_stroke_color(ctx, text_color);
                graphics_draw_round_rect(ctx, frame_mention, 2);
            }
        } else {
            frame_terminus.size.w -= CELL_MARGIN + CELL_SUB_ICON_SIZE + NEXT_TRAIN_CELL_SUB_ICON_RIGHT_MARGIN;
            
            // Draw warning icon
            GRect frame_mention = GRect(frame_terminus.origin.x + frame_terminus.size.w + CELL_MARGIN,
                                        NEXT_TRAIN_CELL_SUB_ICON_Y,
                                        CELL_SUB_ICON_SIZE,
                                        CELL_SUB_ICON_SIZE);
            
            draw_image_in_rect(ctx, is_inverted?RESOURCE_ID_IMG_MENTION_DARK:RESOURCE_ID_IMG_MENTION_LIGHT, frame_mention);
        }
    }
    
    // Draw text, considering the scrolling index
    if (frame_terminus.size.w > 0) {
        draw_text(ctx,
#if TEXT_SCROLL_IS_ENABLED
                  is_selected?text_scroll_text(str_terminus, 0, true, user_info->text_scroll_data):str_terminus,
#else
                  str_terminus,
#endif
                  FONT_KEY_GOTHIC_18,
                  frame_terminus,
                  GTextAlignmentLeft);
    }
    
#if TEXT_SCROLL_IS_ENABLED
    // Scroll texts
    if (is_selected) {
        const char **texts = calloc(1, sizeof(char *));
        texts[0] = str_terminus;
        const GSize **sizes = calloc(1, sizeof(GSize *));
        sizes[0] = &(frame_terminus.size);
        const char **font_keys = calloc(1, sizeof(char *));
        font_keys[0] = FONT_KEY_GOTHIC_18;
        if (!text_scroll_is_unchanged(texts, sizes, font_keys, 1, user_info->text_scroll_data)) {
            Layer *menu_layer = menu_layer_get_layer(user_info->menu_layer);
            text_scroll_create((TextScrollConfig) {
                .frame_interval = TEXT_FRAME_INTERVAL,
                .pause_interval = TEXT_PAUSE_INTERVAL,
                .update_callback = (TextScrollUpdateCb)text_scroll_update,
                .user_context = menu_layer
            },
                               texts,
                               sizes,
                               font_keys,
                               1,
                               &user_info->text_scroll_data);
        }
        free(texts);
        free(sizes);
        free(font_keys);
    }
#endif
}

// MARK: Data

static void release_next_trains_list(NextTrains *user_info) {
    for (int idx = 0; idx < user_info->next_trains_list_count; ++idx) {
        NULL_FREE(user_info->next_trains_list[idx].code);
        NULL_FREE(user_info->next_trains_list[idx].platform);
        NULL_FREE(user_info->next_trains_list[idx].number);
        NULL_FREE(user_info->next_trains_list[idx].mention);
    }
    NULL_FREE(user_info->next_trains_list);
    user_info->next_trains_list_count = 0;
#if EXTRA_INFO_IS_ENABLED
    NULL_FREE(user_info->extra_info);
#endif
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
    if (index == NEXT_TRAINS_ACTIONS_EDIT) {
        return _("Edit");
#if EXTRA_INFO_IS_ENABLED
    } else if (index == NEXT_TRAINS_ACTIONS_EXTRA_INFO) {
        return _("Info");
#endif
    } else {
        return _("Add Favorite");
    }
}

static bool action_list_is_enabled_callback(size_t index, NextTrains *user_info) {
    if (index == NEXT_TRAINS_ACTIONS_FAV) {
        return !fav_exists(user_info->from_to);
#if EXTRA_INFO_IS_ENABLED
    } else if (index == NEXT_TRAINS_ACTIONS_EXTRA_INFO) {
        return user_info->extra_info != NULL && strlen(user_info->extra_info) > 0;
#endif
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index, NextTrains *user_info) {
    if (index == NEXT_TRAINS_ACTIONS_EDIT) {
        ui_push_window(new_window_search_train(user_info->from_to.from, user_info->from_to.to));
#if EXTRA_INFO_IS_ENABLED
    } else if (index == NEXT_TRAINS_ACTIONS_EXTRA_INFO) {
        ui_push_window(new_window_message(user_info->extra_info));
#endif
    } else {
        fav_add(user_info->from_to.from, user_info->from_to.to);
    }
}

// MARK: Click Config Provider

static void window_long_click_handler(ClickRecognizerRef recognizer, void *context) {
    MenuLayer *menu_layer = context;
    MenuIndex selected_index = menu_layer_get_selected_index(menu_layer);
    if (selected_index.section == NEXT_TRAINS_SECTION_INFO) {
        NextTrains *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
        ActionListConfig config = (ActionListConfig){
            .context = user_info,
            .num_rows = NEXT_TRAINS_ACTIONS_COUNT,
#if EXTRA_INFO_IS_ENABLED
            .default_selection = NEXT_TRAINS_ACTIONS_EXTRA_INFO,
#else
            .default_selection = NEXT_TRAINS_ACTIONS_EDIT,
#endif
#ifdef PBL_COLOR
            .colors = {
                .background = HIGHLIGHT_COLOR,
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
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, common_menu_layer_button_up_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30, common_menu_layer_button_down_handler);
}

// MARK: Message Request callbacks

static void message_callback(bool succeeded, NextTrains *user_info, MESSAGE_TYPE type, ...) {
    if (succeeded && type == MESSAGE_TYPE_NEXT_TRAINS) {
        release_next_trains_list(user_info);
        
        va_list ap;
        va_start(ap, type);
        
        user_info->next_trains_list = va_arg(ap, void *);
        user_info->next_trains_list_count = va_arg(ap, size_t);
#if EXTRA_INFO_IS_ENABLED
        user_info->extra_info = va_arg(ap, char *);
#endif
        
        va_end(ap);
        
        // Update UI
#if RELATIVE_TIME_IS_ENABLED
        restart_timers(user_info);
        user_info->show_relative_time = false;
#endif
    } else {
        user_info->next_trains_list_count = -1;
    }
    user_info->is_updating = false;
    menu_layer_reload_data(user_info->menu_layer);
    
    // Feedback
    vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t[]) {50}, .num_segments = 1});
}

static void request_next_stations(NextTrains *user_info) {
    // Update UI
    user_info->is_updating = true;
    menu_layer_reload_data(user_info->menu_layer);
    
    // Send message
    message_send(MESSAGE_TYPE_NEXT_TRAINS,
                 (MessageCallback)message_callback,
                 user_info,
                 user_info->from_to.from,
                 user_info->from_to.to);
}

// MARK: Timers
#if RELATIVE_TIME_IS_ENABLED

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
    layer_mark_dirty(menu_layer_get_layer(user_info->menu_layer));
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

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_sections_callback(struct MenuLayer *menu_layer, NextTrains *user_info) {
    return NEXT_TRAINS_SECTION_COUNT;
}

static uint16_t menu_layer_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, NextTrains *user_info) {
    if (section_index == NEXT_TRAINS_SECTION_INFO || user_info->is_updating) {
        return 1;
    } else if (section_index == NEXT_TRAINS_SECTION_TRAINS) {
        return (user_info->next_trains_list_count > 0)?user_info->next_trains_list_count:1;
    }
    return 0;
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, NextTrains *user_info) {
    return CELL_HEIGHT;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, NextTrains *user_info) {
    bool is_selected = menu_layer_is_index_selected(user_info->menu_layer, cell_index);
    
#ifdef PBL_COLOR
    bool is_inverted = settings_is_dark_theme() || is_selected;
    GColor bg_color = is_selected?HIGHLIGHT_COLOR:curr_bg_color();
    GColor fg_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#else
    bool is_inverted = settings_is_dark_theme()?!is_selected:is_selected;
    GColor bg_color = is_selected?curr_fg_color():curr_bg_color();
    GColor fg_color = is_selected?curr_bg_color():curr_fg_color();
#endif
    
    if (cell_index->section == NEXT_TRAINS_SECTION_INFO) {
        draw_from_to(ctx, cell_layer,
#if TEXT_SCROLL_IS_ENABLED
                     &user_info->text_scroll_data, menu_layer_get_layer(user_info->menu_layer), is_selected,
#endif
                     is_inverted,
                     bg_color, fg_color,
                     user_info->from_to
#if MINI_TIMETABLE_IS_ENABLED
                     ,
                     false
#endif
#if EXTRA_INFO_IS_ENABLED
                     ,
                     (user_info->extra_info != NULL && strlen(user_info->extra_info) > 0)
#endif
                     );
    } else if (cell_index->section == NEXT_TRAINS_SECTION_TRAINS) {
        if (user_info->is_updating) {
            draw_centered_title(ctx, cell_layer,
                                is_selected,
                                _("Loading..."),
                                NULL);
        } else if (user_info->next_trains_list_count > 0) {
            DataModelNextTrain next_train = user_info->next_trains_list[cell_index->row];
            
            // Hour
            char *str_hour = calloc(TIME_STRING_LENGTH, sizeof(char));
            time_2_str(next_train.hour,
                       str_hour,
                       TIME_STRING_LENGTH
#if RELATIVE_TIME_IS_ENABLED
                       ,
                       user_info->show_relative_time
#endif
                       );
            
            // Terminus
            char *str_terminus = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
            stations_get_name(next_train.terminus, str_terminus, STATION_NAME_MAX_LENGTH);
            
            draw_menu_layer_cell(ctx, cell_layer,
                                 fg_color,
                                 is_inverted,
                                 is_selected,
                                 next_train.code,
                                 str_hour,
                                 str_terminus,
                                 next_train.platform,
                                 next_train.mention,
                                 user_info);
            
            // Clean
            free(str_hour);
            free(str_terminus);
        } else if (user_info->next_trains_list_count == 0) {
            draw_centered_title(ctx, cell_layer,
                                is_selected,
                                _("No train."),
                                NULL);
        } else if (user_info->next_trains_list_count == -1) {
            draw_centered_title(ctx, cell_layer,
                                is_selected,
                                _("Request Failed."),
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
            ui_push_window(new_window_train_details(next_train.number, user_info->from_to.from));
        }
    }
}

#if TEXT_SCROLL_IS_ENABLED
static void menu_layer_selection_will_change(struct MenuLayer *menu_layer, MenuIndex *new_index, MenuIndex old_index, NextTrains *user_info) {
    text_scroll_destory(user_info->text_scroll_data);
    user_info->text_scroll_data = NULL;
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
        .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)menu_layer_get_num_sections_callback,
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
    NextTrains *user_info = window_get_user_data(window);
    
    // Data
    NULL_FREE(user_info->str_from);
    NULL_FREE(user_info->str_to);
    release_next_trains_list(user_info);
    
    // Window
    menu_layer_destroy(user_info->menu_layer);
    window_destroy(user_info->window);
    
    NULL_FREE(user_info);
}

static void window_appear(Window *window) {
    // Discard formerly sent requests
    message_clear_callbacks();
    
    NextTrains *user_info = window_get_user_data(window);
    
    // Add status bar
    ui_setup_status_bars(window_get_root_layer(user_info->window), menu_layer_get_layer(user_info->menu_layer));
    
    // Subscribe services
    accel_tap_service_init(accel_tap_service_handler, user_info);
    
    // Request data
    if (user_info->next_trains_list == NULL) {
        request_next_stations(user_info);
    }
    
    // Start timers
#if RELATIVE_TIME_IS_ENABLED
    format_timer_start(user_info);
#endif
    
}

static void window_disappear(Window *window) {
#if TEXT_SCROLL_IS_ENABLED || RELATIVE_TIME_IS_ENABLED
    NextTrains *user_info = window_get_user_data(window);
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

Window* new_window_next_trains(DataModelFromTo from_to) {
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
        
        //Return the window
        return user_info->window;
    }
    return NULL;
}
