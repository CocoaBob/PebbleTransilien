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

#define SERAPATOR_LAYER_HEIGHT 1

enum {
    SEARCH_STATION_ACTIONS_FROM = 0,
    SEARCH_STATION_ACTIONS_TO,
    SEARCH_STATION_ACTIONS_TIMETABLE,
    SEARCH_STATION_ACTIONS_FAV,
    SEARCH_STATION_ACTIONS_INVERT,
    SEARCH_STATION_ACTIONS_COUNT
};

// MARK: Variables

static Window *s_window;
static Layer *s_selection_layer;
static MenuLayer *s_menu_layer;
static Layer *s_menu_layer_layer;
static Layer *s_panel_layer;
static ClickConfigProvider s_last_ccp;
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

static DataModelFromTo s_from_to;

// To deactivate the menu layer
#ifdef PBL_COLOR
static bool s_menu_layer_is_activated;
#else
static InverterLayer *s_inverter_layer_for_first_row;
static Layer *s_inverter_layer_for_first_row_layer;
#endif

// MARK: Forward declaration

static void move_focus_to_selection_layer();
static void move_focus_to_menu_layer();
static bool focus_is_on_selection_layer();
static bool action_list_is_enabled_callback(size_t index);

// MARK: Helpers

static size_t current_search_results_count() {
    size_t return_value = 0;
    if (s_search_results_index >= 0) {
        return_value = s_search_results[s_search_results_index].search_results_count;
    }
    return return_value;
}

static StationIndex current_search_result_at_index(size_t index) {
    size_t return_value = s_search_results[s_search_results_index].search_results[index];
    return return_value;
}

static StationIndex current_search_result() {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    return s_search_results[s_search_results_index].search_results[selected_index.row];
}

static bool value_is_valid(char value) {
    return (value >= SELECTION_LAYER_VALUE_MIN && value <= SELECTION_LAYER_VALUE_MAX);
}

static void verify_from_to(DataModelFromTo *i_o_from_to) {
    if (i_o_from_to->from == STATION_NON && i_o_from_to->to != STATION_NON) {
        i_o_from_to->from = s_from_to.to;
        i_o_from_to->to = STATION_NON;
    }
    if (i_o_from_to->from == STATION_NON) {
        if (!focus_is_on_selection_layer()) {
            i_o_from_to->from = current_search_result();
        }
    }
}

static void reset_search_results() {
    s_search_results_index = -1;
    memset(s_search_string, 0, sizeof(s_search_string));
    for (size_t i = 0; i < SELECTION_LAYER_CELL_COUNT; ++i) {
        s_search_results[i].search_results_count = 0;
        for (size_t result = 0; result < STATION_SEARH_RESULT_MAX_COUNT; ++result) {
            s_search_results[i].search_results[result] = STATION_NON;
        }
    }
    menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, 0), MenuRowAlignTop, false);
}

// MARK: Drawing

static void panel_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    // Draw background
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    // Draw border
    graphics_context_set_stroke_color(ctx, curr_fg_color());
    graphics_draw_round_rect(ctx, GRect(1, 1, bounds.size.w - 2, bounds.size.h - 1), 3);
    
    // Draw stations
    DataModelFromTo *from_to = layer_get_data(layer);
#ifdef PBL_COLOR
    bool is_dark_theme = status_is_dark_theme();
#endif
    GColor text_color = curr_fg_color();
    
    draw_from_to_layer(ctx,
                       layer,
                       *from_to,
#ifdef PBL_COLOR
                       is_dark_theme,
#endif
                       text_color);
}

static void separator_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, curr_fg_color());
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

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

// MARK: Info Panel

static bool panel_is_visible() {
    return layer_get_window(s_panel_layer) != NULL;
}

static void panel_show() {
    if (!panel_is_visible()) {
        GRect menu_layer_frame = layer_get_frame(s_menu_layer_layer);
        GRect panel_layer_frame = layer_get_frame(s_panel_layer);
        
        menu_layer_frame.size.h -= panel_layer_frame.size.h;
        layer_set_frame(s_menu_layer_layer, menu_layer_frame);
        
        panel_layer_frame.origin.y = menu_layer_frame.origin.y + menu_layer_frame.size.h;
        layer_set_frame(s_panel_layer, panel_layer_frame);
        
        Layer *window_layer = window_get_root_layer(s_window);
        layer_add_child(window_layer, s_panel_layer);
    }
}

static void panel_hide() {
    if (panel_is_visible()) {
        GRect menu_layer_frame = layer_get_frame(s_menu_layer_layer);
        GRect panel_layer_frame = layer_get_frame(s_panel_layer);
        
        menu_layer_frame.size.h += panel_layer_frame.size.h;
        layer_set_frame(s_menu_layer_layer, menu_layer_frame);
        
        layer_remove_from_parent(s_panel_layer);
    }
}

static void panel_update(DataModelFromTo i_from_to) {
    DataModelFromTo *from_to = layer_get_data(s_panel_layer);
    if (i_from_to.from == STATION_NON && i_from_to.to != STATION_NON) {
        from_to->from = i_from_to.to;
        from_to->to = i_from_to.from;
    } else {
        from_to->from = i_from_to.from;
        from_to->to = i_from_to.to;
    }
    
    if (i_from_to.from == STATION_NON && i_from_to.to == STATION_NON) {
        panel_hide();
    } else {
        layer_mark_dirty(s_panel_layer);
        panel_show();
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
    return SEARCH_STATION_ACTIONS_COUNT;
}

static size_t action_list_get_default_selection_callback(void) {
    // If timetable is avaiable to check, select timetable by default
    if (action_list_is_enabled_callback(SEARCH_STATION_ACTIONS_TIMETABLE)) {
        return SEARCH_STATION_ACTIONS_TIMETABLE;
    } else {
        // Otherwise select anyone which is available
        for (int i = 0; i < SEARCH_STATION_ACTIONS_COUNT; ++i) {
            if (action_list_is_enabled_callback(i)) {
                return i;
            }
        }
        // Or just the 1st one
        return 0;
    }
}

static char* action_list_get_title_callback(size_t index) {
    switch (index) {
        case SEARCH_STATION_ACTIONS_FROM:
        {
            if (focus_is_on_selection_layer()) {
                return "Clear Departure";
            } else {
                return "Set Departure";
            }
        }
        case SEARCH_STATION_ACTIONS_TO:
        {
            if (focus_is_on_selection_layer()) {
                return "Clear Destination";
            } else {
                return "Set Destination";
            }
        }
        case SEARCH_STATION_ACTIONS_TIMETABLE:
            return "Check Timetable";
        case SEARCH_STATION_ACTIONS_FAV:
            return "Set Favorite";
        case SEARCH_STATION_ACTIONS_INVERT:
            return "Invert";
        default:
            return "";
    }
}

static bool action_list_is_enabled_callback(size_t index) {
    switch (index) {
        case SEARCH_STATION_ACTIONS_FROM:
        {
            if (focus_is_on_selection_layer()) {
                return (s_from_to.from != STATION_NON); // If departure station has been set
            } else {
                return true; // Always available to set depature
            }
        }
        case SEARCH_STATION_ACTIONS_TO:
        {
            if (focus_is_on_selection_layer()) {
                return (s_from_to.to != STATION_NON); // If destination station has been set
            } else {
                return (s_from_to.from != STATION_NON && // If depature is set, then we can set the destination
                        s_from_to.from != current_search_result()); // If the current selected station isn't the same
            }
        }
        case SEARCH_STATION_ACTIONS_TIMETABLE:
        {
            if (focus_is_on_selection_layer()) {
                return (s_from_to.from != STATION_NON || s_from_to.to != STATION_NON); // If there's at least one station
            } else {
                return true; // Always available to check the current selected station's timetable
            }
        }
        case SEARCH_STATION_ACTIONS_FAV:
        {
            DataModelFromTo from_to = s_from_to;
            verify_from_to(&from_to);
            if (focus_is_on_selection_layer()) {
                if (from_to.from == STATION_NON && from_to.to == STATION_NON) {
                    return false; // If the favorite isn't valid, we don't save it
                } else {
                    return !fav_exists(from_to); // If the favorite exists, we don't save it again
                }
            } else {
                if (from_to.from == STATION_NON && from_to.to == STATION_NON) {
                    return (current_search_result() != STATION_NON); // When from_to is invalid, save the current selected one
                } else {
                    return true; // When from_to is valid, save from_to
                }
            }
        }
        case SEARCH_STATION_ACTIONS_INVERT:
        {
            return (s_from_to.from != STATION_NON || s_from_to.to != STATION_NON);
        }
        default:
            return true;
        break;
    }
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    switch (index) {
        case SEARCH_STATION_ACTIONS_FROM:
        {
            if (focus_is_on_selection_layer()) {
                s_from_to.from = STATION_NON;
            } else {
                s_from_to.from = current_search_result();
            }
            reset_search_results();
            move_focus_to_selection_layer();
            window_stack_remove(action_list_window, true);
            panel_update(s_from_to);
            break;
        }
        case SEARCH_STATION_ACTIONS_TO:
        {
            if (focus_is_on_selection_layer()) {
                s_from_to.to = STATION_NON;
            } else {
                s_from_to.to = current_search_result();
            }
            reset_search_results();
            move_focus_to_selection_layer();
            window_stack_remove(action_list_window, true);
            panel_update(s_from_to);
            break;
        }
        case SEARCH_STATION_ACTIONS_TIMETABLE:
        {
            DataModelFromTo from_to = s_from_to;
            verify_from_to(&from_to);
            window_stack_remove(action_list_window, false);
            if (from_to.from != STATION_NON) {
                push_next_trains_window(from_to, true);
            } else if (from_to.to != STATION_NON) {
                push_next_trains_window((DataModelFromTo){from_to.to,STATION_NON}, true);
            }
            break;
        }
        case SEARCH_STATION_ACTIONS_FAV:
        {
            verify_from_to(&s_from_to);
            fav_add(s_from_to.from, s_from_to.to);
            window_stack_remove(action_list_window, true);
            break;
        }
        case SEARCH_STATION_ACTIONS_INVERT:
        {
            StationIndex from = s_from_to.from;
            s_from_to.from = s_from_to.to;
            s_from_to.to = from;
            panel_update(s_from_to);
            window_stack_remove(action_list_window, true);
            break;
        }
        default:
            break;
    }
}

// MARK: Click Config Provider

static void long_select_handler(ClickRecognizerRef recognizer, void *context) {
    action_list_present_with_callbacks((ActionListCallbacks) {
        .get_bar_color = (ActionListGetBarColorCallback)action_list_get_bar_color,
        .get_num_rows = (ActionListGetNumberOfRowsCallback)action_list_get_num_rows_callback,
        .get_default_selection = (ActionListGetDefaultSelectionCallback)action_list_get_default_selection_callback,
        .get_title = (ActionListGetTitleCallback)action_list_get_title_callback,
        .is_enabled = (ActionListIsEnabledCallback)action_list_is_enabled_callback,
        .select_click = (ActionListSelectCallback)action_list_select_callback
    });
}

static void click_config_provider_for_selection_layer(void *context) {
    s_last_ccp(context);
    window_long_click_subscribe(BUTTON_ID_SELECT, 0, long_select_handler, NULL);
}

static void button_back_handler(ClickRecognizerRef recognizer, void *context) {
    move_focus_to_selection_layer();
}

static void click_config_provider_for_menu_layer(void *context) {
    s_last_ccp(context);
    window_single_click_subscribe(BUTTON_ID_BACK, button_back_handler);
}

// MARK: Move focus

static bool focus_is_on_selection_layer() {
#ifdef PBL_COLOR
    return !s_menu_layer_is_activated;
#else
    return layer_get_window(s_inverter_layer_for_first_row_layer) != NULL;
#endif
}

static void move_focus_to_selection_layer() {
    selection_layer_set_active(s_selection_layer, true);
    
    // Setup Click Config Providers
    selection_layer_set_click_config_onto_window(s_selection_layer, s_window);
    s_last_ccp = window_get_click_config_provider(s_window);
    window_set_click_config_provider_with_context(s_window, click_config_provider_for_selection_layer, s_selection_layer);
    
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
    s_last_ccp = window_get_click_config_provider(s_window);
    window_set_click_config_provider_with_context(s_window, click_config_provider_for_menu_layer, s_menu_layer);
    
#ifdef PBL_COLOR
    s_menu_layer_is_activated = true;
    set_menu_layer_activated(s_menu_layer, true);
#else
    set_menu_layer_activated(s_menu_layer, true, s_inverter_layer_for_first_row);
#endif
    
    // Display the selected station
    panel_update(s_from_to);
}

// MARK: Selection layer callbacks

static char* selection_handle_get_text(int index, void *context) {
    return &s_search_string[index];
}

static void selection_handle_will_change(int old_index, int *new_index, bool is_forward, void *context) {
    if (!is_forward) {
        // Return to the main menu window
        if (old_index == 0) {
            window_stack_pop(true);
        }
        // Clear all indexes behind
        else {
            for (int i = old_index; i <= SELECTION_LAYER_CELL_COUNT; ++i) {
                s_search_string[i] = '\0';
            }
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

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    size_t return_value = current_search_results_count();
    if (return_value == 0) {
        return 1;
    } else {
        return return_value;
    }
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT_2;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
#ifdef PBL_BW
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    GRect row_frame = layer_get_frame(cell_layer);
    if (focus_is_on_selection_layer() && // Need to hide the highlighted row
        menu_index_compare(cell_index, &selected_index) == 0) { // It's the highlighted row
        layer_set_frame(s_inverter_layer_for_first_row_layer, row_frame);
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
        draw_cell_title(ctx, cell_layer, (s_search_results_index >= 0)?"Not found.":"UP/DOWN to input...");
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    action_list_present_with_callbacks((ActionListCallbacks) {
        .get_bar_color = (ActionListGetBarColorCallback)action_list_get_bar_color,
        .get_num_rows = (ActionListGetNumberOfRowsCallback)action_list_get_num_rows_callback,
        .get_default_selection = (ActionListGetDefaultSelectionCallback)action_list_get_default_selection_callback,
        .get_title = (ActionListGetTitleCallback)action_list_get_title_callback,
        .is_enabled = (ActionListIsEnabledCallback)action_list_is_enabled_callback,
        .select_click = (ActionListSelectCallback)action_list_select_callback
    });
}

static void menu_layer_selection_changed_callback(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) {
    if (s_from_to.from == STATION_NON && s_from_to.to == STATION_NON) {
        StationIndex station_index = current_search_result_at_index(new_index.row);
        panel_update((DataModelFromTo){station_index, STATION_NON});
    }
}

static int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return SEPARATOR_HEIGHT;
}

static void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)  {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

#ifdef PBL_PLATFORM_BASALT
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
    int16_t status_bar_height = 0;
#ifdef PBL_PLATFORM_BASALT
    status_bar_height = STATUS_BAR_LAYER_HEIGHT;
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer, &s_status_bar_overlay_layer);
#endif
    
    // Add separator between selection layer and menu layer
    Layer *separator_layer = layer_create(GRect(0, status_bar_height + SELECTION_LAYER_HEIGHT, window_bounds.size.w, SERAPATOR_LAYER_HEIGHT));
    layer_set_update_proc(separator_layer, separator_layer_proc);
    layer_add_child(window_layer, separator_layer);
    
    // Add menu layer
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + status_bar_height + SELECTION_LAYER_HEIGHT + SERAPATOR_LAYER_HEIGHT,
                                   window_bounds.size.w,
                                   window_bounds.size.h - status_bar_height - SELECTION_LAYER_HEIGHT - SERAPATOR_LAYER_HEIGHT);
    s_menu_layer = menu_layer_create(menu_layer_frame);
    s_menu_layer_layer = menu_layer_get_layer(s_menu_layer);
    layer_add_child(window_layer, s_menu_layer_layer);
    
    // Setup menu layer
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback,
        .selection_changed = (MenuLayerSelectionChangedCallback)menu_layer_selection_changed_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)menu_layer_draw_separator_callback
#ifdef PBL_PLATFORM_BASALT
        ,
        .draw_background = (MenuLayerDrawBackgroundCallback)menu_layer_draw_background_callback
#endif
    });
    
    // Add panel layer
    s_panel_layer = layer_create_with_data(GRect(window_bounds.origin.x,
                                                 window_bounds.origin.y + window_bounds.size.h - CELL_HEIGHT,
                                                 window_bounds.size.w,
                                                 CELL_HEIGHT),
                                           sizeof(DataModelFromTo));
    layer_set_update_proc(s_panel_layer, panel_layer_proc);
    panel_update(s_from_to);
    
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
    
    // Add inverter layer for Aplite
#ifdef PBL_BW
    s_inverter_layer = inverter_layer_create(window_bounds);
    s_inverter_layer_for_first_row = inverter_layer_create(window_bounds);
    s_inverter_layer_for_first_row_layer = inverter_layer_get_layer(s_inverter_layer_for_first_row);
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
#ifdef PBL_PLATFORM_BASALT
    stations_search_name_begin();
#endif
}

static void window_disappear(Window *window) {
#ifdef PBL_PLATFORM_BASALT
    stations_search_name_end();
#endif
}

// MARK: Entry point

void push_search_train_window(StationIndex from, StationIndex to, bool animated) {
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
    reset_search_results();
    s_from_to.from = from;
    s_from_to.to = to;
    
    move_focus_to_selection_layer();
}
