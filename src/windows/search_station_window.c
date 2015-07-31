//
//  search_station_window.c
//  PebbleTransilien
//
//  Created by CocoaBob on 25/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Constants

#define SELECTION_LAYER_HEIGHT 22
#define SELECTION_LAYER_CELL_COUNT 8
#define SELECTION_LAYER_VALUE_MIN 'A'
#define SELECTION_LAYER_VALUE_MAX 'Z'

enum {
    SEARCH_STATION_ACTIONS_ANOTHER = 0,
    SEARCH_STATION_ACTIONS_TIMETABLE,
    SEARCH_STATION_ACTIONS_FAV,
    SEARCH_STATION_ACTIONS_COUNT
};

// MARK: Variables

static Window *s_window;
static Layer *s_selection_layer;
static MenuLayer *s_menu_layer;
static ClickConfigProvider s_ccp_of_menu_layer;
#ifdef PBL_PLATFORM_BASALT
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
static Layer *s_status_bar_overlay_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

// Searching
static char s_search_string[SELECTION_LAYER_CELL_COUNT+1] = {0};

typedef struct SearchStationResult {
    StationIndex search_results[STATION_SEARH_RESULT_MAX_COUNT];
    size_t search_results_count;
} SearchStationResult;
static SearchStationResult s_search_results[SELECTION_LAYER_CELL_COUNT];
static int s_search_results_index;

// To deactivate the menu layer
#ifdef PBL_COLOR
static bool s_menu_layer_is_activated;
#else
static InverterLayer *s_inverter_layer_for_first_row;
#endif

// MARK: Forward declaration

static void move_focus_to_selection_layer();
static void move_focus_to_menu_layer();

// MARK: Searching helpers

size_t current_search_results_count() {
    size_t return_value = 0;
    if (s_search_results_index >= 0) {
        return_value = s_search_results[s_search_results_index].search_results_count;
    }
    return return_value;
}

StationIndex current_search_result_at_index(size_t index) {
    size_t return_value = s_search_results[s_search_results_index].search_results[index];
    return return_value;
}

StationIndex current_search_result() {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    return s_search_results[s_search_results_index].search_results[selected_index.row];
}

bool value_is_valid(char value) {
    return (value >= SELECTION_LAYER_VALUE_MIN && value <= SELECTION_LAYER_VALUE_MAX);
}

// MARK: Drawing

static void draw_menu_layer_cell(GContext *ctx, Layer *cell_layer,
                                 GColor text_color,
#ifdef PBL_COLOR
                                 bool is_highlighed,
#endif
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
                                bounds.size.w - CELL_MARGIN * 2 - FROM_TO_ICON_WIDTH,
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
    return SEARCH_STATION_ACTIONS_COUNT;
}

static size_t action_list_get_default_selection_callback(void) {
    return SEARCH_STATION_ACTIONS_TIMETABLE;
}

static char* action_list_get_title_callback(size_t index) {
    switch (index) {
        case SEARCH_STATION_ACTIONS_ANOTHER:
            return "Search another";
            break;
        case SEARCH_STATION_ACTIONS_TIMETABLE:
            return "Timetable";
            break;
        case SEARCH_STATION_ACTIONS_FAV:
            return "Favorite";
            break;
        default:
            return "";
            break;
    }
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    StationIndex selected_station_index = current_search_result();
    switch (index) {
        case SEARCH_STATION_ACTIONS_ANOTHER:
            window_stack_remove(action_list_window, false);
            window_stack_remove(s_window, false);
            push_search_train_window(true);
            break;
        case SEARCH_STATION_ACTIONS_TIMETABLE:
            window_stack_remove(action_list_window, false);
            push_next_trains_window((DataModelFromTo){selected_station_index, STATION_NON}, true);
            break;
        case SEARCH_STATION_ACTIONS_FAV:
            // TODO:
            fav_add(selected_station_index, STATION_NON);
            window_stack_remove(action_list_window, true);
            break;
        default:
            break;
    }
}

// MARK: Click Config Provider

static void button_back_handler(ClickRecognizerRef recognizer, void *context) {
    move_focus_to_selection_layer();
}

static void click_config_provider(void *context) {
    s_ccp_of_menu_layer(context);
    window_single_click_subscribe(BUTTON_ID_BACK, button_back_handler);
}

// MARK: Move focus

static void move_focus_to_selection_layer() {
    selection_layer_set_click_config_onto_window(s_selection_layer, s_window);
    selection_layer_set_active(s_selection_layer, true);
    
    // Deactivate the menu layer
#ifdef PBL_COLOR
    s_menu_layer_is_activated = false;
    set_menu_layer_activated(s_menu_layer, false);
#else
    set_menu_layer_activated(s_menu_layer, false, s_inverter_layer_for_first_row);
#endif
}

static void move_focus_to_menu_layer() {
    selection_layer_set_active(s_selection_layer, false);
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
    s_ccp_of_menu_layer = window_get_click_config_provider(s_window);
    window_set_click_config_provider_with_context(s_window, click_config_provider, s_menu_layer);
    
#ifdef PBL_COLOR
    s_menu_layer_is_activated = true;
    set_menu_layer_activated(s_menu_layer, true);
#else
    set_menu_layer_activated(s_menu_layer, true, s_inverter_layer_for_first_row);
#endif
}

// MARK: Selection layer callbacks

static char* selection_handle_get_text(int index, void *context) {
    return &s_search_string[index];
}

static void selection_handle_will_change(int old_index, int *new_index, bool is_forward, void *context) {
    if (!is_forward) {
        // Return to the last window
        if (old_index == 0) {
            window_stack_pop(true);
        }
        // Clear last index
        else {
            s_search_string[old_index] = '\0';
        }
    } else if (is_forward) {
        // If no result for the last index, and the value of current index (old_index) is empty
        // Forbid to forward, because we have nothing to select in the list
        if (current_search_results_count() == 0 && !value_is_valid(s_search_string[old_index])) {
            *new_index = old_index;
        }
        // If it's the last index
        else if (old_index == SELECTION_LAYER_CELL_COUNT - 1 ||
                 // Or we want to skip 2 indexes to go the list directly
                 (old_index > 0 && !value_is_valid(s_search_string[old_index]) && !value_is_valid(s_search_string[old_index - 1])))
        {
            // To hide the highlight of selection layer
            *new_index = SELECTION_LAYER_CELL_COUNT;
            
            move_focus_to_menu_layer();
        }
    }
}

static void selection_handle_did_change(int index, bool is_forward, void *context) {
    // We update the list only if the value at index-1 is valid
    if (value_is_valid(s_search_string[index - 1])) {
        s_search_results_index = index - 1;
        if (is_forward) {
            StationIndex *search_results = (StationIndex *)&s_search_results[s_search_results_index].search_results;
            size_t *search_results_count = (size_t *)&s_search_results[s_search_results_index].search_results_count;
            stations_search_name(s_search_string, search_results, STATION_SEARH_RESULT_MAX_COUNT, search_results_count);
        }
        menu_layer_reload_data(s_menu_layer);
    }
}

static void selection_handle_inc(int index, uint8_t clicks, void *context) {
    if (index != 0 &&
        // If the value at index-1 isn't valid, forbid to choose
        (!value_is_valid(s_search_string[index - 1]) ||
         // Or if the results for last index is 0, no need to continue
         current_search_results_count() == 0)) {
            return;
        }
    
    if (!value_is_valid(s_search_string[index])) {
        s_search_string[index] = SELECTION_LAYER_VALUE_MIN;
    } else {
        ++s_search_string[index];
        if (s_search_string[index] > SELECTION_LAYER_VALUE_MAX) {
            s_search_string[index] = SELECTION_LAYER_VALUE_MIN;
        }
    }
}

static void selection_handle_dec(int index, uint8_t clicks, void *context) {
    if (index != 0 &&
        // If the value at index-1 isn't valid, forbid to choose
        (!value_is_valid(s_search_string[index - 1]) ||
         // Or if the results for last index is 0, no need to continue
         current_search_results_count() == 0)) {
            return;
        }
    
    if (!value_is_valid(s_search_string[index])) {
        s_search_string[index] = SELECTION_LAYER_VALUE_MAX;
    } else {
        --s_search_string[index];
        if (s_search_string[index] < SELECTION_LAYER_VALUE_MIN) {
            s_search_string[index] = SELECTION_LAYER_VALUE_MAX;
        }
    }
}

// MARK: Menu layer callbacks

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    size_t return_value = current_search_results_count();
    if (return_value == 0) {
        return 1;
    } else {
        return return_value;
    }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT_2;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
#ifdef PBL_BW
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    GRect row_frame = layer_get_frame(cell_layer);
    if (menu_index_compare(cell_index, &selected_index) == 0 &&
        layer_get_window(inverter_layer_get_layer(s_inverter_layer_for_first_row))) {
        layer_set_frame(inverter_layer_get_layer(s_inverter_layer_for_first_row), row_frame);
    }
#endif

#ifdef PBL_COLOR
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = s_menu_layer_is_activated?(menu_index_compare(&selected_index, cell_index) == 0):false;
    bool is_dark_theme = status_is_dark_theme();
    bool is_highlighed = is_dark_theme || is_selected;
    GColor text_color = (is_selected && !is_dark_theme)?curr_bg_color():curr_fg_color();
#else
    GColor text_color = curr_fg_color();
#endif
    
    if (current_search_results_count() > 0) {
        StationIndex station_index = current_search_result_at_index(cell_index->row);
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(station_index, str_station, STATION_NAME_MAX_LENGTH);
        
        draw_menu_layer_cell(ctx, cell_layer,
                             text_color,
#ifdef PBL_COLOR
                             is_highlighed,
#endif
                             str_station);
        
        // Clean
        free(str_station);
    } else {
        graphics_context_set_text_color(ctx, text_color);
        draw_cell_title(ctx, cell_layer, (s_search_results_index > 0)?"Not found.":"UP/DOWN to input...");
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
    
    // Prepare status bar
    int16_t status_bar_height = 0;
#ifdef PBL_PLATFORM_BASALT
    status_bar_height = STATUS_BAR_LAYER_HEIGHT;
#endif
    
    // Add menu layer
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + status_bar_height + SELECTION_LAYER_HEIGHT + 1,
                                   window_bounds.size.w,
                                   window_bounds.size.h - status_bar_height - SELECTION_LAYER_HEIGHT - 1);
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

    
    // Add status bar
#ifdef PBL_PLATFORM_BASALT
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer, &s_status_bar_overlay_layer);
#endif
    
    // Add selection layer
    s_selection_layer = selection_layer_create(GRect(0, status_bar_height, window_bounds.size.w, SELECTION_LAYER_HEIGHT), SELECTION_LAYER_CELL_COUNT);
    int selection_layer_cell_width = window_bounds.size.w / SELECTION_LAYER_CELL_COUNT;
    for (int i = 0; i < SELECTION_LAYER_CELL_COUNT; ++i) {
        selection_layer_set_cell_width(s_selection_layer, i, selection_layer_cell_width);
    }
    selection_layer_set_cell_padding(s_selection_layer, 0);
    selection_layer_set_font(s_selection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
#ifdef PBL_COLOR
    selection_layer_set_active_bg_color(s_selection_layer, GColorCobaltBlue);
    selection_layer_set_inactive_bg_color(s_selection_layer, GColorDarkGray);
#endif
    selection_layer_set_callbacks(s_selection_layer, NULL, (SelectionLayerCallbacks) {
        .get_cell_text = selection_handle_get_text,
        .will_change = selection_handle_will_change,
        .did_change = selection_handle_did_change,
        .increment = selection_handle_inc,
        .decrement = selection_handle_dec,
    });
    layer_add_child(window_layer, s_selection_layer);
    
    // Finally, ass inverter layer for Aplite
#ifdef PBL_BW
    s_inverter_layer = inverter_layer_create(window_bounds);
    s_inverter_layer_for_first_row = inverter_layer_create(window_bounds);
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
    inverter_layer_destroy(s_inverter_layer_for_first_row);
#endif
}

static void window_appear(Window *window) {
    stations_search_name_begin();
}

static void window_disappear(Window *window) {
    stations_search_name_end();
}

// MARK: Entry point

void push_search_train_window(bool animated) {
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
    }
    
    window_stack_push(s_window, animated);
    
    // Initialize Data
    s_search_results_index = -1;
    memset(s_search_string, 0, sizeof(s_search_string));
    for (size_t i = 0; i < SELECTION_LAYER_CELL_COUNT; ++i) {
        s_search_results[i].search_results_count = 0;
    }
    
    move_focus_to_selection_layer();
}
