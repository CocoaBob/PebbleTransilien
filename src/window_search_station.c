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
#define SELECTION_LAYER_SEPARATOR_HEIGHT 1
#define SELECTION_LAYER_HEIGHT 22
#define SELECTION_LAYER_CELL_COUNT 8
#define SELECTION_LAYER_VALUE_MIN 'A'
#define SELECTION_LAYER_VALUE_MAX 'Z'

enum {
    SEARCH_STATION_ACTIONS_REVERSE,
    SEARCH_STATION_ACTIONS_DESTINATION,
    SEARCH_STATION_ACTIONS_TIMETABLE,
    SEARCH_STATION_ACTIONS_FAV,
    SEARCH_STATION_ACTIONS_COUNT
};

enum {
    SEARCH_STATION_SELECTION_LAYER,
    SEARCH_STATION_MENU_LAYER,
    SEARCH_STATION_PANEL_LAYER
};

typedef struct PanelData {
    bool is_active;
    DataModelFromTo from_to;
} PanelData;

typedef struct SearchStationResult {
    StationIndex search_results[STATION_SEARH_RESULT_MAX_COUNT];
    size_t search_results_count;
} SearchStationResult;

typedef struct {
    Window *window;
    Layer *selection_layer;
    MenuLayer *menu_layer;
    Layer *panel_layer;
    ClickConfigProvider last_ccp;
    // Searching
    char search_string[SELECTION_LAYER_CELL_COUNT+1];
    SearchStationResult search_results[SELECTION_LAYER_CELL_COUNT];
    int search_results_index;
    
    DataModelFromTo from_to;
    
    // To deactivate the menu layer
    uint8_t active_layer_index;
    
    bool is_initialised;
} SearchStation;

// MARK: Forward declaration

static void move_focus_to_selection_layer(SearchStation *user_info);
static void move_focus_to_menu_layer(SearchStation *user_info);
static void move_focus_to_panel(SearchStation *user_info);

// MARK: Helpers

static size_t current_search_results_count(SearchStation *user_info) {
    size_t return_value = 0;
    if (user_info->search_results_index >= 0) {
        return_value = user_info->search_results[user_info->search_results_index].search_results_count;
    }
    return return_value;
}

static StationIndex current_search_result_at_index(size_t index, SearchStation *user_info) {
    if (index < user_info->search_results[user_info->search_results_index].search_results_count) {
        return user_info->search_results[user_info->search_results_index].search_results[index];
    }
    return STATION_NON;
}

static StationIndex current_search_result(SearchStation *user_info) {
    return current_search_result_at_index(menu_layer_get_selected_index(user_info->menu_layer).row, user_info);
}

static bool value_is_valid(char value) {
    return (value >= SELECTION_LAYER_VALUE_MIN && value <= SELECTION_LAYER_VALUE_MAX);
}

static void confirm_from_to(DataModelFromTo *i_o_from_to, SearchStation *user_info) {
    StationIndex search_result = current_search_result(user_info);
    if (search_result != STATION_NON) {
        if (i_o_from_to->from == STATION_NON) {
            i_o_from_to->from = search_result;
            i_o_from_to->to = STATION_NON;
        } else {
            i_o_from_to->to = search_result;
        }
    }
}

static void reset_search_results(SearchStation *user_info) {
    user_info->search_results_index = -1;
    memset(user_info->search_string, 0, sizeof(user_info->search_string));
    for (size_t i = 0; i < SELECTION_LAYER_CELL_COUNT; ++i) {
        user_info->search_results[i].search_results_count = 0;
        for (size_t result = 0; result < STATION_SEARH_RESULT_MAX_COUNT; ++result) {
            user_info->search_results[i].search_results[result] = STATION_NON;
        }
    }
    menu_layer_set_selected_index(user_info->menu_layer, MenuIndex(0, 0), MenuRowAlignTop, false);
}

// MARK: Drawing

static void panel_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    PanelData *layer_data = layer_get_data(layer);
    
    // Background or Highlight
    GColor bg_color;
    if (layer_data->is_active) {
        bg_color = PBL_IF_COLOR_ELSE(HIGHLIGHT_COLOR, curr_fg_color());
    } else {
        bg_color = PBL_IF_COLOR_ELSE(GColorDarkGray, curr_bg_color());
    }
    // Draw background
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    // Draw highlight border
    if (layer_data->is_active) {
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_draw_rect(ctx, GRect(bounds.origin.x, bounds.origin.y + 1, bounds.size.w, bounds.size.h - 1));
    }
    
    // Separator
    graphics_context_set_stroke_color(ctx, curr_fg_color());
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));
    
    // Draw stations
    GColor fg_color = layer_data->is_active?curr_bg_color():curr_fg_color();
    draw_from_to(ctx, layer,
#if TEXT_SCROLL_IS_ENABLED
                 layer, layer_data->is_active,
#endif
#ifdef PBL_COLOR
                 true,
                 bg_color, GColorWhite,
#else
                 settings_is_dark_theme()?!layer_data->is_active:layer_data->is_active,
                 bg_color, fg_color,
#endif
                 layer_data->from_to
#if MINI_TIMETABLE_IS_ENABLED
                 ,
                 false
#endif
                 );
}

// MARK: Info Panel

static bool panel_is_visible(SearchStation *user_info) {
    return layer_get_window(user_info->panel_layer) != NULL;
}

static void panel_show(SearchStation *user_info) {
    if (!panel_is_visible(user_info)) {
        GRect menu_layer_frame = layer_get_frame(menu_layer_get_layer(user_info->menu_layer));
        GRect panel_layer_frame = layer_get_frame(user_info->panel_layer);
        
        menu_layer_frame.size.h -= panel_layer_frame.size.h;
        layer_set_frame(menu_layer_get_layer(user_info->menu_layer), menu_layer_frame);
        
        panel_layer_frame.origin.y = menu_layer_frame.origin.y + menu_layer_frame.size.h;
        layer_set_frame(user_info->panel_layer, panel_layer_frame);
        
        Layer *window_layer = window_get_root_layer(user_info->window);
        layer_add_child(window_layer, user_info->panel_layer);
    }
}

static void panel_hide(SearchStation *user_info) {
    if (panel_is_visible(user_info)) {
        GRect menu_layer_frame = layer_get_frame(menu_layer_get_layer(user_info->menu_layer));
        GRect panel_layer_frame = layer_get_frame(user_info->panel_layer);
        
        menu_layer_frame.size.h += panel_layer_frame.size.h;
        layer_set_frame(menu_layer_get_layer(user_info->menu_layer), menu_layer_frame);
        
        layer_remove_from_parent(user_info->panel_layer);
    }
}

static void panel_update(DataModelFromTo i_from_to, SearchStation *user_info) {
    PanelData *layer_data = layer_get_data(user_info->panel_layer);
    DataModelFromTo *from_to = &layer_data->from_to;
    from_to->from = i_from_to.from;
    from_to->to = i_from_to.to;
    
    if (i_from_to.from != STATION_NON || i_from_to.to != STATION_NON) {
        layer_mark_dirty(user_info->panel_layer);
        panel_show(user_info);
    } else {
        panel_hide(user_info);
    }
}

static void panel_update_with_menu_layer_selection(SearchStation *user_info) {
    if (user_info->from_to.from == STATION_NON) {
        panel_update((DataModelFromTo){current_search_result(user_info), STATION_NON}, user_info);
    } else {
        panel_update((DataModelFromTo){user_info->from_to.from, current_search_result(user_info)}, user_info);
    }
}

// MARK: Action list callbacks

static char* action_list_get_title_callback(size_t index, SearchStation *user_info) {
    if (index == SEARCH_STATION_ACTIONS_REVERSE) {
        return _("Reverse");
    } else if (index == SEARCH_STATION_ACTIONS_DESTINATION) {
        return _("Add Destination");
    } else if (index == SEARCH_STATION_ACTIONS_TIMETABLE) {
        return _("Check Timetable");
    } else {
        return _("Add Favorite");
    }
}

static bool action_list_is_enabled_callback(size_t index, SearchStation *user_info) {
    if (index == SEARCH_STATION_ACTIONS_FAV) {
        DataModelFromTo from_to = user_info->from_to;
        confirm_from_to(&from_to, user_info);
        return !fav_exists(from_to);
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index, SearchStation *user_info) {
    DataModelFromTo from_to = user_info->from_to;
    confirm_from_to(&from_to, user_info);
    if (index == SEARCH_STATION_ACTIONS_REVERSE) {
        user_info->from_to.from = from_to.to;
        user_info->from_to.to = from_to.from;
    } else if (index == SEARCH_STATION_ACTIONS_DESTINATION) {
        user_info->from_to = from_to;
        if (user_info->from_to.to != STATION_NON) {
            user_info->from_to.from = user_info->from_to.to;
            user_info->from_to.to = STATION_NON;
        }
    } else if (index == SEARCH_STATION_ACTIONS_FAV) {
        fav_add(from_to.from, from_to.to);
        window_stack_pop(true);
        return;
    } else { // SEARCH_STATION_ACTIONS_TIMETABLE
        ui_push_window(new_window_next_trains(from_to));
        return;
    }
    // Update UI for SEARCH_STATION_ACTIONS_REVERSE & SEARCH_STATION_ACTIONS_DESTINATION
    panel_update(user_info->from_to, user_info);
    move_focus_to_selection_layer(user_info);
    reset_search_results(user_info);
}

static void show_action_list(SearchStation *user_info) {
    ActionListConfig config = (ActionListConfig){
        .context = user_info,
        .num_rows = SEARCH_STATION_ACTIONS_COUNT,
        .default_selection = SEARCH_STATION_ACTIONS_TIMETABLE,
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

// MARK: Click Config Provider

// Menu layer
static void button_back_handler_for_menu_layer(ClickRecognizerRef recognizer, void *context) {
    MenuLayer *menu_layer = context;
    SearchStation *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
    move_focus_to_selection_layer(user_info);
}

static void click_config_provider_for_menu_layer(void *context) {
    MenuLayer *menu_layer = context;
    SearchStation *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
    user_info->last_ccp(user_info->menu_layer);
    window_single_click_subscribe(BUTTON_ID_BACK, button_back_handler_for_menu_layer);
}

// Panel layer
static void button_back_handler_for_panel_layer(ClickRecognizerRef recognizer, void *context) {
    MenuLayer *menu_layer = context;
    SearchStation *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
    if (current_search_results_count(user_info) > 0) {
        move_focus_to_menu_layer(user_info);
    } else {
        move_focus_to_selection_layer(user_info);
    }
}

static void button_select_handler_for_panel_layer(ClickRecognizerRef recognizer, void *context) {
    MenuLayer *menu_layer = context;
    SearchStation *user_info = window_get_user_data(layer_get_window(menu_layer_get_layer(menu_layer)));
    show_action_list(user_info);
}

static void click_config_provider_for_panel_layer(void *context) {
    window_single_click_subscribe(BUTTON_ID_BACK, button_back_handler_for_panel_layer);
    window_single_click_subscribe(BUTTON_ID_UP, button_back_handler_for_panel_layer);
    window_single_click_subscribe(BUTTON_ID_DOWN, button_select_handler_for_panel_layer);
    window_single_click_subscribe(BUTTON_ID_SELECT, button_select_handler_for_panel_layer);
}

// MARK: Move focus

static void search_selection_layer_set_active(bool is_active, SearchStation *user_info) {
    menu_layer_set_selected_index(user_info->menu_layer, (MenuIndex){0,0}, MenuRowAlignCenter, false);
    
    // Update selection layer
    selection_layer_set_active(user_info->selection_layer, is_active);
    
    if (is_active) {
        // Display departure & destination
        panel_update(user_info->from_to, user_info);
    }
}

static void menu_layer_set_active(bool is_active, SearchStation *user_info) {
    // Update menu layer
    menu_layer_set_highlight_colors(user_info->menu_layer,
                                    is_active ? PBL_IF_COLOR_ELSE(HIGHLIGHT_COLOR, curr_fg_color()) : curr_bg_color(),
                                    is_active ? PBL_IF_COLOR_ELSE(GColorWhite,      curr_bg_color()) : curr_fg_color());
    
    if (is_active) {
        // Display selected station if departure & destination are STATION_NON
        panel_update_with_menu_layer_selection(user_info);
    }
}


static void panel_layer_set_active(bool is_active, SearchStation *user_info) {
#if TEXT_SCROLL_IS_ENABLED
    // Stop scrolling text
    text_scroll_end();
#endif
    
    // Update panel
    PanelData *layer_data = layer_get_data(user_info->panel_layer);
    layer_data->is_active = is_active;
    layer_mark_dirty(user_info->panel_layer);
}

static void move_focus_to_selection_layer(SearchStation *user_info) {
    // Move
    user_info->active_layer_index = SEARCH_STATION_SELECTION_LAYER;
    
    search_selection_layer_set_active(true, user_info);
    menu_layer_set_active(false, user_info);
    panel_layer_set_active(false, user_info);
    
    // Setup Click Config Providers
    selection_layer_set_click_config_onto_window(user_info->selection_layer, user_info->window);
}

static void move_focus_to_menu_layer(SearchStation *user_info) {
    // Move
    user_info->active_layer_index = SEARCH_STATION_MENU_LAYER;
    
    search_selection_layer_set_active(false, user_info);
    menu_layer_set_active(true, user_info);
    panel_layer_set_active(false, user_info);
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(user_info->menu_layer, user_info->window);
    user_info->last_ccp = window_get_click_config_provider(user_info->window);
    window_set_click_config_provider_with_context(user_info->window, click_config_provider_for_menu_layer, user_info->menu_layer);
}

static void move_focus_to_panel(SearchStation *user_info) {
    // Move
    user_info->active_layer_index = SEARCH_STATION_PANEL_LAYER;
    
    search_selection_layer_set_active(false, user_info);
    menu_layer_set_active(false, user_info);
    panel_layer_set_active(true, user_info);
    
    // Setup Click Config Providers
    window_set_click_config_provider_with_context(user_info->window, click_config_provider_for_panel_layer, user_info->panel_layer);
}

// MARK: Selection layer callbacks

static char* selection_handle_get_text(int index, SearchStation *user_info) {
    return &user_info->search_string[index];
}

static void selection_handle_will_change(int old_index, int *new_index, bool is_forward, SearchStation *user_info) {
    if (!is_forward) {
        // Return to the main menu window
        if (old_index == 0) {
            // Mark -1 to quit
            *new_index = -1;
        }
        // Clear all indexes behind
        else {
            for (int i = old_index; i <= SELECTION_LAYER_CELL_COUNT; ++i) {
                user_info->search_string[i] = '\0';
            }
        }
    } else if (is_forward) {
        // If no result for the last index, and the value of current index (old_index) is empty
        if (current_search_results_count(user_info) == 0 && !value_is_valid(user_info->search_string[old_index])) {
            if (panel_is_visible(user_info)) {
                // Move focus to panel, to confirm the favorite
                move_focus_to_panel(user_info);
            } else {
                // Forbid to forward, because we have nothing to select in the list
                *new_index = old_index;
            }
        }
        // If it's the last index
        else if (old_index == SELECTION_LAYER_CELL_COUNT - 1 ||
                 // Or we want to skip 2 indexes to go the list directly
                 (old_index > 0 && !value_is_valid(user_info->search_string[old_index]) && !value_is_valid(user_info->search_string[old_index - 1])))
        {
            move_focus_to_menu_layer(user_info);
        }
    }
}

static void selection_handle_did_change(int index, bool is_forward, SearchStation *user_info) {
    // If index == -1, return to the Main Menu window
    if (index == -1) {
        window_stack_pop(true);
    }
    // We update the list only if the value at index-1 is valid
    if (value_is_valid(user_info->search_string[index - 1])) {
        user_info->search_results_index = index - 1;
        if (is_forward) {
            StationIndex *search_results = (StationIndex *)&user_info->search_results[user_info->search_results_index].search_results;
            size_t *search_results_count = (size_t *)&user_info->search_results[user_info->search_results_index].search_results_count;
            stations_search_name(user_info->search_string, search_results, STATION_SEARH_RESULT_MAX_COUNT, search_results_count);
        }
        menu_layer_reload_data(user_info->menu_layer);
    }
}

static void selection_change(int index, bool is_increase, SearchStation *user_info) {
    if (index != 0 &&
        // If the value at index-1 isn't valid, forbid to choose
        (!value_is_valid(user_info->search_string[index - 1]) ||
         // Or if the results for last index is 0, no need to continue
         current_search_results_count(user_info) == 0)) {
            return;
        }
    
    if (!value_is_valid(user_info->search_string[index])) {
        user_info->search_string[index] = is_increase?SELECTION_LAYER_VALUE_MIN:SELECTION_LAYER_VALUE_MAX;
    } else {
        user_info->search_string[index] += is_increase?1:-1;
        if (user_info->search_string[index] > SELECTION_LAYER_VALUE_MAX) {
            user_info->search_string[index] = SELECTION_LAYER_VALUE_MIN;
        } else if (user_info->search_string[index] < SELECTION_LAYER_VALUE_MIN) {
            user_info->search_string[index] = SELECTION_LAYER_VALUE_MAX;
        }
    }
}

static void selection_handle_inc(int index, uint8_t clicks, SearchStation *user_info) {
    selection_change(index, true, user_info);
}

static void selection_handle_dec(int index, uint8_t clicks, SearchStation *user_info) {
    selection_change(index, false, user_info);
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, SearchStation *user_info) {
    return current_search_results_count(user_info)?:1;
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, SearchStation *user_info) {
    return CELL_HEIGHT_2;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, SearchStation *user_info) {
    bool is_selected = (user_info->active_layer_index == SEARCH_STATION_MENU_LAYER)?menu_layer_is_index_selected(user_info->menu_layer, cell_index):false;
    bool is_highlighted = settings_is_dark_theme() || is_selected;
    
#ifdef PBL_COLOR
    bool is_inverted = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#else
    bool is_inverted = settings_is_dark_theme()?!is_selected:is_selected;
    GColor text_color = is_selected?curr_bg_color():curr_fg_color();
#endif
    
    if (current_search_results_count(user_info) > 0) {
        StationIndex station_index = current_search_result_at_index(cell_index->row, user_info);
        
        // Station
        char *str_station = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(station_index, str_station, STATION_NAME_MAX_LENGTH);
        
        draw_station(ctx, cell_layer,
#if TEXT_SCROLL_IS_ENABLED
                     menu_layer_get_layer(user_info->menu_layer), is_selected,
#endif
                     text_color,
                     is_inverted,
                     NULL,
                     str_station);
        
        // Clean
        free(str_station);
    } else {
        draw_centered_title(ctx, cell_layer,
                            (user_info->active_layer_index != SEARCH_STATION_MENU_LAYER) || is_highlighted,
                            (user_info->search_results_index >= 0)?_("Not found."):_("Press UP/DOWN"),
                            NULL);
    }
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, SearchStation *user_info) {
    if (current_search_results_count(user_info) > 0) {
        show_action_list(user_info);
    }
}

static void menu_layer_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, SearchStation *user_info) {
    if (panel_is_visible(user_info)) {
        move_focus_to_panel(user_info);
    } else {
        show_action_list(user_info);
    }
}

static void menu_layer_selection_changed_callback(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, SearchStation *user_info) {
#if TEXT_SCROLL_IS_ENABLED
    text_scroll_end();
#endif
    if (user_info->active_layer_index == SEARCH_STATION_MENU_LAYER) {
        panel_update_with_menu_layer_selection(user_info);
    }
}

// MARK: Window callbacks

static void window_load(Window *window) {
    SearchStation *user_info = window_get_user_data(window);
    
    // Window
    Layer *window_layer = window_get_root_layer(user_info->window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add separator between selection layer and menu layer
    // To save memory, we just change the window background's color
    window_set_background_color(user_info->window, curr_fg_color());
    
    // Add menu layer
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + STATUS_BAR_LAYER_HEIGHT + SELECTION_LAYER_HEIGHT + SELECTION_LAYER_SEPARATOR_HEIGHT,
                                   window_bounds.size.w,
                                   window_bounds.size.h - STATUS_BAR_LAYER_HEIGHT - SELECTION_LAYER_HEIGHT - SELECTION_LAYER_SEPARATOR_HEIGHT);
    
    user_info->menu_layer = menu_layer_create(menu_layer_frame);
#ifdef PBL_ROUND
//    menu_layer_set_center_focused(user_info->menu_layer, false);
#endif
    layer_add_child(window_layer, menu_layer_get_layer(user_info->menu_layer));
    
    // Setup menu layer
    menu_layer_pad_bottom_enable(user_info->menu_layer, false);
    menu_layer_set_callbacks(user_info->menu_layer, user_info, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback,
        .select_long_click = (MenuLayerSelectCallback)menu_layer_select_long_callback,
        .selection_changed = (MenuLayerSelectionChangedCallback)menu_layer_selection_changed_callback
        ,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)common_menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)common_menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)common_menu_layer_draw_background_callback
    });
    
    // Add panel layer
    user_info->panel_layer = layer_create_with_data(GRect(window_bounds.origin.x,
                                                 window_bounds.origin.y + window_bounds.size.h - CELL_HEIGHT,
                                                 window_bounds.size.w,
                                                 CELL_HEIGHT),
                                           sizeof(PanelData));
    layer_set_update_proc(user_info->panel_layer, panel_layer_proc);
    
    // Add selection layer
    GRect selection_layer_frame = GRect(0, STATUS_BAR_LAYER_HEIGHT, window_bounds.size.w, SELECTION_LAYER_HEIGHT);
#ifdef PBL_ROUND
    GPoint selection_layer_offset = GPoint(0, selection_layer_frame.origin.y + selection_layer_frame.size.h / 2);
    selection_layer_offset.x = get_round_border_x_radius_82(selection_layer_offset.y - CELL_MARGIN_2);
    selection_layer_frame.origin.x += selection_layer_offset.x + CELL_MARGIN_2;
    selection_layer_frame.size.w -= selection_layer_offset.x + selection_layer_offset.x + CELL_MARGIN_4;
#endif
    user_info->selection_layer = selection_layer_create(selection_layer_frame, SELECTION_LAYER_CELL_COUNT);
    
    
    int selection_layer_cell_width = selection_layer_frame.size.w / SELECTION_LAYER_CELL_COUNT;
    for (int i = 0; i < SELECTION_LAYER_CELL_COUNT; ++i) {
        selection_layer_set_cell_width(user_info->selection_layer, i, selection_layer_cell_width);
    }
    selection_layer_set_cell_padding(user_info->selection_layer, 0);
    selection_layer_set_font(user_info->selection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    selection_layer_set_inactive_color(user_info->selection_layer,  PBL_IF_COLOR_ELSE(GColorWhite, curr_fg_color()), PBL_IF_COLOR_ELSE(GColorDarkGray,   curr_bg_color()));
    selection_layer_set_active_color(user_info->selection_layer,    PBL_IF_COLOR_ELSE(GColorWhite, curr_bg_color()), PBL_IF_COLOR_ELSE(HIGHLIGHT_COLOR, curr_fg_color()));
    
    selection_layer_set_callbacks(user_info->selection_layer, user_info, (SelectionLayerCallbacks) {
        .get_cell_text = (SelectionLayerGetCellText)selection_handle_get_text,
        .will_change = (SelectionLayerWillChangeCallback)selection_handle_will_change,
        .did_change = (SelectionLayerDidChangeCallback)selection_handle_did_change,
        .increment = (SelectionLayerIncrementCallback)selection_handle_inc,
        .decrement = (SelectionLayerDecrementCallback)selection_handle_dec,
    });
    layer_add_child(window_layer, user_info->selection_layer);
    
    // Setup theme
    ui_setup_theme(user_info->window, user_info->menu_layer);
}

static void window_unload(Window *window) {
    SearchStation *user_info = window_get_user_data(window);
    
    // Window
    selection_layer_destroy(user_info->selection_layer);
    layer_destroy(user_info->panel_layer);
    menu_layer_destroy(user_info->menu_layer);
    window_destroy(user_info->window);
    
    free(user_info);
}

static void window_appear(Window *window) {
    SearchStation *user_info = window_get_user_data(window);
    
    if (!user_info->is_initialised) {
        user_info->is_initialised = true;
        
        // Initialize Data
        reset_search_results(user_info);
        panel_update(user_info->from_to, user_info);
        
        // Focus the selection layer
        move_focus_to_selection_layer(user_info);
    }
    
    // Add status bar
    ui_setup_status_bar(window_get_root_layer(user_info->window), menu_layer_get_layer(user_info->menu_layer));
    
#if !defined(PBL_PLATFORM_APLITE)
    // High performance solution
    stations_search_name_begin();
#endif
}

static void window_disappear(Window *window) {
#if TEXT_SCROLL_IS_ENABLED
    // Stop scrolling text
    text_scroll_end();
#endif
    
#if !defined(PBL_PLATFORM_APLITE)
    // High performance solution
    // Release memory for searching
    stations_search_name_end();
#endif
}

// MARK: Entry point

Window* new_window_search_train(StationIndex from, StationIndex to) {
    SearchStation *user_info = calloc(1, sizeof(SearchStation));
    if (user_info) {
        user_info->window = window_create();
        window_set_user_data(user_info->window, user_info);
        window_set_window_handlers(user_info->window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .disappear = window_disappear,
            .unload = window_unload,
        });
        
        user_info->from_to.from = from;
        user_info->from_to.to = to;
        
        // Return the window
        return user_info->window;
    }
    return NULL;
}