//
//  window_search_station.c
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
    SEARCH_STATION_ACTIONS_DESTINATION,
    SEARCH_STATION_ACTIONS_TIMETABLE,
    SEARCH_STATION_ACTIONS_FAV,
    SEARCH_STATION_ACTIONS_COUNT
};

enum {
    SEARCH_STATION_SELECTION_LAYER = 0,
    SEARCH_STATION_MENU_LAYER,
    SEARCH_STATION_PANEL_LAYER
};

typedef struct PanelData {
    bool is_active;
    DataModelFromTo from_to;
} PanelData;

// MARK: Variables

static Window *s_window;
static Layer *s_selection_layer;
static MenuLayer *s_menu_layer;
static Layer *s_menu_layer_layer;
static Layer *s_panel_layer;
static ClickConfigProvider s_last_ccp;
#if !defined(PBL_PLATFORM_APLITE)
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
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
static uint8_t s_actived_layer_index;

// MARK: Forward declaration

static void move_focus_to_selection_layer();
static void move_focus_to_menu_layer();
static void move_focus_to_panel();

// MARK: Helpers

static size_t current_search_results_count() {
    size_t return_value = 0;
    if (s_search_results_index >= 0) {
        return_value = s_search_results[s_search_results_index].search_results_count;
    }
    return return_value;
}

static StationIndex current_search_result_at_index(size_t index) {
    return s_search_results[s_search_results_index].search_results[index];
}

static StationIndex current_search_result() {
    return current_search_result_at_index(menu_layer_get_selected_index(s_menu_layer).row);
}

static bool value_is_valid(char value) {
    return (value >= SELECTION_LAYER_VALUE_MIN && value <= SELECTION_LAYER_VALUE_MAX);
}

static void confirm_from_to(DataModelFromTo *i_o_from_to) {
    if (i_o_from_to->from == STATION_NON) {
        i_o_from_to->from = current_search_result();
        i_o_from_to->to = STATION_NON;
    } else {
        i_o_from_to->to = current_search_result();
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
    PanelData *layer_data = layer_get_data(s_panel_layer);
    
#ifdef PBL_COLOR
    // Background or Highlight
    if (layer_data->is_active) {
        graphics_context_set_fill_color(ctx, GColorCobaltBlue);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_draw_rect(ctx, GRect(bounds.origin.x, bounds.origin.y + 1, bounds.size.w, bounds.size.h - 1));
    } else {
        graphics_context_set_fill_color(ctx, GColorDarkGray);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }
    
    // Separator
    graphics_context_set_stroke_color(ctx, curr_fg_color());
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));
#else
    // Background or Highlight
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    if (layer_data->is_active) {
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_draw_rect(ctx, grect_crop(bounds, 1));
    } else {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, grect_crop(bounds, 1), 0, GCornerNone);
    }
#endif
    
    // Draw stations
#ifdef PBL_COLOR
    GColor text_color = GColorWhite;
#endif
    
    draw_from_to(ctx, layer,
#ifdef PBL_COLOR
                 true,
                 text_color,
#else
                 layer_data->is_active,
#endif
                 layer_data->from_to);
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
        
#ifdef PBL_COLOR
        Layer *window_layer = window_get_root_layer(s_window);
        layer_add_child(window_layer, s_panel_layer);
#else
        // To make sure the panel layer is under the inverter layer
        // But we can't just insert it below the inverter layer which isn't always there
        layer_insert_above_sibling(s_panel_layer, menu_layer_get_layer(s_menu_layer));
#endif
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
    PanelData *layer_data = layer_get_data(s_panel_layer);
    DataModelFromTo *from_to = &layer_data->from_to;
    from_to->from = i_from_to.from;
    from_to->to = i_from_to.to;
    
    if (i_from_to.from != STATION_NON || i_from_to.to != STATION_NON) {
        layer_mark_dirty(s_panel_layer);
        panel_show();
    } else {
        panel_hide();
    }
}

static void panel_update_with_menu_layer_selection() {
    if (s_from_to.from == STATION_NON) {
        panel_update((DataModelFromTo){current_search_result(), STATION_NON});
    } else {
        panel_update((DataModelFromTo){s_from_to.from, current_search_result()});
    }
}

// MARK: Action list callbacks

static char* action_list_get_title_callback(size_t index) {
    if (index == SEARCH_STATION_ACTIONS_DESTINATION) {
        return _("Add Destination");
    } else if (index == SEARCH_STATION_ACTIONS_TIMETABLE) {
        return _("Check Timetable");
    } else {
        return _("Set Favorite");
    }
}

static bool action_list_is_enabled_callback(size_t index) {
    if (index == SEARCH_STATION_ACTIONS_FAV) {
        DataModelFromTo from_to = s_from_to;
        confirm_from_to(&from_to);
        return !fav_exists(from_to);
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    DataModelFromTo from_to = s_from_to;
    confirm_from_to(&from_to);
    switch (index) {
        case SEARCH_STATION_ACTIONS_DESTINATION:
        {
            s_from_to = from_to;
            if (s_from_to.to != STATION_NON) {
                s_from_to.from = s_from_to.to;
                s_from_to.to = STATION_NON;
            }
            panel_update(s_from_to);
            move_focus_to_selection_layer();
            reset_search_results();
            window_stack_remove(action_list_window, true);
            break;
        }
        case SEARCH_STATION_ACTIONS_TIMETABLE:
        {
            window_stack_remove(action_list_window, false);
            push_window_next_trains(from_to, true);
            break;
        }
        case SEARCH_STATION_ACTIONS_FAV:
        {
            fav_add(from_to.from, from_to.to);
            window_stack_remove(action_list_window, true);
            break;
        }
    }
}

static void show_action_list() {
    ActionListConfig config = (ActionListConfig){
        .num_rows = SEARCH_STATION_ACTIONS_COUNT,
        .default_selection = SEARCH_STATION_ACTIONS_TIMETABLE,
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

// MARK: Click Config Provider

// Menu layer
static void button_back_handler_for_menu_layer(ClickRecognizerRef recognizer, void *context) {
    move_focus_to_selection_layer();
}

static void click_config_provider_for_menu_layer(void *context) {
    s_last_ccp(context);
    window_single_click_subscribe(BUTTON_ID_BACK, button_back_handler_for_menu_layer);
}

// Panel layer
static void button_back_handler_for_panel_layer(ClickRecognizerRef recognizer, void *context) {
    if (current_search_results_count() > 0) {
        move_focus_to_menu_layer();
    } else {
        move_focus_to_selection_layer();
    }
}

static void button_select_handler_for_panel_layer(ClickRecognizerRef recognizer, void *context) {
    show_action_list();
}

static void click_config_provider_for_panel_layer(void *context) {
    window_single_click_subscribe(BUTTON_ID_BACK, button_back_handler_for_panel_layer);
    window_single_click_subscribe(BUTTON_ID_UP, button_back_handler_for_panel_layer);
    window_single_click_subscribe(BUTTON_ID_DOWN, button_select_handler_for_panel_layer);
    window_single_click_subscribe(BUTTON_ID_SELECT, button_select_handler_for_panel_layer);
}

// MARK: Move focus

static void search_selection_layer_set_active(bool is_active) {
    selection_layer_set_active(s_selection_layer, is_active);
    
    if (is_active) {
        // Display departure & destination
        panel_update(s_from_to);
    }
}

static void menu_layer_set_active(bool is_active) {
#ifdef PBL_COLOR
    set_menu_layer_activated(s_menu_layer, is_active);
#endif
    
    if (is_active) {
        // Display selected station if departure & destination are STATION_NON
        panel_update_with_menu_layer_selection();
    }
#ifdef PBL_BW
    else {
        // Select the first row to make it easier to set inverter layer's position
        menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, 0), MenuRowAlignTop, false);
    }
#endif
}


static void panel_layer_set_active(bool is_active) {
    PanelData *layer_data = layer_get_data(s_panel_layer);
    layer_data->is_active = is_active;
    layer_mark_dirty(s_panel_layer);
}

static void move_focus_to_selection_layer() {
    s_actived_layer_index = SEARCH_STATION_SELECTION_LAYER;
    
    search_selection_layer_set_active(true);
    
    menu_layer_set_active(false);
    
    panel_layer_set_active(false);
    
    // Setup Click Config Providers
    selection_layer_set_click_config_onto_window(s_selection_layer, s_window);
}

static void move_focus_to_menu_layer() {
    s_actived_layer_index = SEARCH_STATION_MENU_LAYER;
    
    search_selection_layer_set_active(false);
    
    menu_layer_set_active(true);
    
    panel_layer_set_active(false);
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, s_window);
    s_last_ccp = window_get_click_config_provider(s_window);
    window_set_click_config_provider_with_context(s_window, click_config_provider_for_menu_layer, s_menu_layer);
}

static void move_focus_to_panel() {
    s_actived_layer_index = SEARCH_STATION_PANEL_LAYER;
    
    search_selection_layer_set_active(false);
    
    menu_layer_set_active(false);
    
    panel_layer_set_active(true);
    
    // Setup Click Config Providers
    window_set_click_config_provider_with_context(s_window, click_config_provider_for_panel_layer, NULL);
}

// MARK: Selection layer callbacks

static char* selection_handle_get_text(int index, void *context) {
    return &s_search_string[index];
}

static void selection_handle_will_change(int old_index, int *new_index, bool is_forward, void *context) {
    if (!is_forward) {
        // Return to the main menu window
        if (old_index == 0) {
#if defined(PBL_PLATFORM_APLITE)
            // Remove main menu window to reduce memory for Aplite.
            window_stack_remove(s_window, true);
            push_window_main_menu(false);
#else
            window_stack_pop(true);
#endif
        }
        // Clear all indexes behind
        else {
            for (int i = old_index; i <= SELECTION_LAYER_CELL_COUNT; ++i) {
                s_search_string[i] = '\0';
            }
        }
    } else if (is_forward) {
        // If no result for the last index, and the value of current index (old_index) is empty
        if (current_search_results_count() == 0 && !value_is_valid(s_search_string[old_index])) {
            if (panel_is_visible()) {
                // Move focus to panel, to confirm the favorite
                move_focus_to_panel();
            } else {
                // Forbid to forward, because we have nothing to select in the list
                *new_index = old_index;
            }
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

static void selection_change(int index, bool is_increase) {
    if (index != 0 &&
        // If the value at index-1 isn't valid, forbid to choose
        (!value_is_valid(s_search_string[index - 1]) ||
         // Or if the results for last index is 0, no need to continue
         current_search_results_count() == 0)) {
            return;
        }
    
    if (!value_is_valid(s_search_string[index])) {
        s_search_string[index] = is_increase?SELECTION_LAYER_VALUE_MIN:SELECTION_LAYER_VALUE_MAX;
    } else {
        s_search_string[index] += is_increase?1:-1;
        if (s_search_string[index] > SELECTION_LAYER_VALUE_MAX) {
            s_search_string[index] = SELECTION_LAYER_VALUE_MIN;
        } else if (s_search_string[index] < SELECTION_LAYER_VALUE_MIN) {
            s_search_string[index] = SELECTION_LAYER_VALUE_MAX;
        }
    }
}

static void selection_handle_inc(int index, uint8_t clicks, void *context) {
    selection_change(index, true);
}

static void selection_handle_dec(int index, uint8_t clicks, void *context) {
    selection_change(index, false);
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    return current_search_results_count()?:1;
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT_2;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
#ifdef PBL_COLOR
    bool is_selected = (s_actived_layer_index == SEARCH_STATION_MENU_LAYER)?(selected_index.row == cell_index->row):false;
    bool is_highlighed = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#endif
    
    if (current_search_results_count() > 0) {
        StationIndex station_index = current_search_result_at_index(cell_index->row);
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(station_index, str_station, STATION_NAME_MAX_LENGTH);
        
        draw_station(ctx, cell_layer,
#ifdef PBL_COLOR
                     text_color,
                     is_highlighed,
#else               
                     (s_actived_layer_index == SEARCH_STATION_SELECTION_LAYER) && (selected_index.row == cell_index->row),
#endif
                     NULL,
                     str_station);
        
        // Clean
        free(str_station);
    } else {
        draw_centered_title(ctx, cell_layer,
                            (s_search_results_index >= 0)?_("Not found."):_("Press UP/DOWN"),
                            NULL);
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (current_search_results_count() > 0) {
        show_action_list();
    }
}

static void menu_layer_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (panel_is_visible()) {
        move_focus_to_panel();
    } else {
        show_action_list();
    }
}

static void menu_layer_selection_changed_callback(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) {
    if (s_actived_layer_index == SEARCH_STATION_MENU_LAYER) {
        panel_update_with_menu_layer_selection();
    }
}

// MARK: Window callbacks

static void window_load(Window *window) {
    // Window
    Layer *window_layer = window_get_root_layer(s_window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    
    // Add status bar
    int16_t status_bar_height = 0;
#if !defined(PBL_PLATFORM_APLITE)
    status_bar_height = STATUS_BAR_LAYER_HEIGHT;
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer);
#endif
    
    // Add separator between selection layer and menu layer
    // To save memory, we just change the window background's color
#if !defined(PBL_PLATFORM_APLITE)
    window_set_background_color(s_window, curr_fg_color());
#endif
    
    // Add menu layer
#if defined(PBL_PLATFORM_APLITE)
    uint8_t separator_height = 0;
#else
    uint8_t separator_height = 1;
#endif
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + status_bar_height + SELECTION_LAYER_HEIGHT + separator_height,
                                   window_bounds.size.w,
                                   window_bounds.size.h - status_bar_height - SELECTION_LAYER_HEIGHT - separator_height);
    
    s_menu_layer = menu_layer_create(menu_layer_frame);
    s_menu_layer_layer = menu_layer_get_layer(s_menu_layer);
    layer_add_child(window_layer, s_menu_layer_layer);
    
    // Setup menu layer
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback,
        .select_long_click = (MenuLayerSelectCallback)menu_layer_select_long_callback,
        .selection_changed = (MenuLayerSelectionChangedCallback)menu_layer_selection_changed_callback
#if !defined(PBL_PLATFORM_APLITE)
        ,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)menu_layer_draw_background_callback
#endif
    });
    
    // Add panel layer
    s_panel_layer = layer_create_with_data(GRect(window_bounds.origin.x,
                                                 window_bounds.origin.y + window_bounds.size.h - CELL_HEIGHT,
                                                 window_bounds.size.w,
                                                 CELL_HEIGHT),
                                           sizeof(PanelData));
    layer_set_update_proc(s_panel_layer, panel_layer_proc);
    
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
        .get_cell_text = (SelectionLayerGetCellText)selection_handle_get_text,
        .will_change = (SelectionLayerWillChangeCallback)selection_handle_will_change,
        .did_change = (SelectionLayerDidChangeCallback)selection_handle_did_change,
        .increment = (SelectionLayerIncrementCallback)selection_handle_inc,
        .decrement = (SelectionLayerDecrementCallback)selection_handle_dec,
    });
    layer_add_child(window_layer, s_selection_layer);
    
    // Prepare inverter layers for Aplite
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
#if !defined(PBL_PLATFORM_APLITE)
    stations_search_name_begin();
#endif
}

static void window_disappear(Window *window) {
#if !defined(PBL_PLATFORM_APLITE)
    stations_search_name_end();
#endif
}

// MARK: Entry point

void push_window_search_train(StationIndex from, StationIndex to, bool animated) {
    // Show window
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
    panel_update(s_from_to);
    
    // Focus the selection layer
    move_focus_to_selection_layer();
}

#if defined(PBL_PLATFORM_APLITE)
Window *get_window_search_train() {
    return s_window;
}
#endif