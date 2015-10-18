//
//  window_main_menu.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

enum {
    MAIN_MENU_SECTION_FAV,
    MAIN_MENU_SECTION_SEARCH,
    MAIN_MENU_SECTION_SETTING,
    MAIN_MENU_SECTION_ABOUT,
    MAIN_MENU_SECTION_COUNT
};

enum {
//    MAIN_MENU_SECTION_SEARCH_ROW_NEARBY = 0,
    MAIN_MENU_SECTION_SEARCH_ROW_NAME,
    MAIN_MENU_SECTION_SEARCH_ROW_COUNT
};

enum {
    MAIN_MENU_SECTION_SETTING_ROW_THEME,
    MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE,
    MAIN_MENU_SECTION_SETTING_ROW_ON_LAUNCH,
    MAIN_MENU_SECTION_SETTING_ROW_COUNT
};

enum {
    MAIN_MENU_SECTION_ABOUT_ROW_AUTHOR,
    MAIN_MENU_SECTION_ABOUT_ROW_VERSION,
    MAIN_MENU_SECTION_ABOUT_ROW_COUNT
};

enum {
    MAIN_MENU_ACTIONS_MOVE_UP,
    MAIN_MENU_ACTIONS_EDIT,
    MAIN_MENU_ACTIONS_DELETE,
    MAIN_MENU_ACTIONS_COUNT
};

static Window *s_window;
static MenuLayer *s_menu_layer;
#if !defined(PBL_PLATFORM_APLITE)
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

// MARK: Forward declaration

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context);

// MARK: Action list callbacks

static char* action_list_get_title_callback(size_t index) {
    if (index == MAIN_MENU_ACTIONS_MOVE_UP) {
        return _("Move up");
    } else if (index == MAIN_MENU_ACTIONS_EDIT) {
        return _("Edit Favorite");
    } else {
        return _("Delete Favorite");
    }
}

static bool action_list_is_enabled_callback(size_t index) {
    if (index == MAIN_MENU_ACTIONS_MOVE_UP) {
        return (fav_get_count() > 1 && menu_layer_get_selected_index(s_menu_layer).row > 0);
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index) {
    MenuIndex current_selection = menu_layer_get_selected_index(s_menu_layer);
    switch (index) {
        case MAIN_MENU_ACTIONS_MOVE_UP:
        {
            fav_move_up_index(current_selection.row);
            window_stack_remove(action_list_window, true);
            menu_layer_set_selected_next(s_menu_layer, true, MenuRowAlignCenter, false);
            break;
        }
        case MAIN_MENU_ACTIONS_EDIT:
        {
            Favorite favorite = fav_at_index(current_selection.row);
            window_stack_remove(action_list_window, false);
            push_window_search_train(favorite.from, favorite.to, true);
            break;
        }
        case MAIN_MENU_ACTIONS_DELETE:
        {
            fav_delete_at_index(current_selection.row);
            window_stack_remove(action_list_window, true);
            break;
        }
    }
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return MAIN_MENU_SECTION_COUNT;
}

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == MAIN_MENU_SECTION_FAV) {
        return fav_get_count();
    } else if (section_index == MAIN_MENU_SECTION_SEARCH) {
        return MAIN_MENU_SECTION_SEARCH_ROW_COUNT;
    } else if (section_index == MAIN_MENU_SECTION_SETTING) {
        return MAIN_MENU_SECTION_SETTING_ROW_COUNT;
    } else {
        return MAIN_MENU_SECTION_ABOUT_ROW_COUNT;
    }
}

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT;
}

static int16_t menu_layer_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == MAIN_MENU_SECTION_FAV) {
        return fav_get_count()?HEADER_HEIGHT:0;
    }
    return HEADER_HEIGHT;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
    bool is_fav_on_launch = settings_is_fav_on_launch();
#ifdef PBL_COLOR
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (menu_index_compare(&selected_index, cell_index) == 0);
    bool is_highlighed = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#endif
    uint16_t section = cell_index->section;
    uint16_t row = cell_index->row;
    if (section == MAIN_MENU_SECTION_FAV ) {
        Favorite favorite = fav_at_index(row);
        draw_from_to(ctx, cell_layer,
#ifdef PBL_COLOR
                     is_highlighed,
                     text_color,
#else
                     false,
#endif
                     favorite);
    } else if (section == MAIN_MENU_SECTION_SEARCH) {
        /*if (row == MAIN_MENU_SECTION_SEARCH_ROW_NEARBY) {
            menu_cell_basic_draw(ctx, cell_layer, "Nearby...", _("Based on location"), NULL);
        } else */if (row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            menu_cell_basic_draw(ctx, cell_layer, _("Alphabetic..."), _("By choosing letters"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_SETTING) {
        if (row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            menu_cell_basic_draw(ctx, cell_layer, _("Theme"), settings_is_dark_theme()?_("Dark theme"):_("Light theme"), NULL);
        } else if (row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            menu_cell_basic_draw(ctx, cell_layer, "Language", _("English"), NULL);
        } else if (row == MAIN_MENU_SECTION_SETTING_ROW_ON_LAUNCH) {
            menu_cell_basic_draw(ctx, cell_layer, _("On launch"), is_fav_on_launch?_("Show 1st favorit"):_("Show home"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_ABOUT) {
        if (row == MAIN_MENU_SECTION_ABOUT_ROW_AUTHOR) {
            menu_cell_basic_draw(ctx, cell_layer, _("Developer"), "@CocoaBob", NULL);
        } else if (row == MAIN_MENU_SECTION_ABOUT_ROW_VERSION) {
            menu_cell_basic_draw(ctx, cell_layer, _("Version"), "1.0.0", NULL);
        }
    }
}

static void menu_layer_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
#ifdef PBL_COLOR
    if (section_index == MAIN_MENU_SECTION_FAV) {
        draw_menu_header(ctx, cell_layer, _("Favorites"), curr_fg_color());
    } else if (section_index == MAIN_MENU_SECTION_SEARCH) {
        draw_menu_header(ctx, cell_layer, _("Search"), curr_fg_color());
    } else if (section_index == MAIN_MENU_SECTION_SETTING) {
        draw_menu_header(ctx, cell_layer, _("Settings"), curr_fg_color());
    } else if (section_index == MAIN_MENU_SECTION_ABOUT) {
        draw_menu_header(ctx, cell_layer, _("About"), curr_fg_color());
    }
#else
    if (section_index == MAIN_MENU_SECTION_FAV) {
        menu_cell_basic_header_draw(ctx, cell_layer, _("Favorites"));
    } else if (section_index == MAIN_MENU_SECTION_SEARCH) {
        menu_cell_basic_header_draw(ctx, cell_layer, _("Search"));
    } else if (section_index == MAIN_MENU_SECTION_SETTING) {
        menu_cell_basic_header_draw(ctx, cell_layer, _("Settings"));
    } else if (section_index == MAIN_MENU_SECTION_ABOUT) {
        menu_cell_basic_header_draw(ctx, cell_layer, _("About"));
    }
#endif
}

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (cell_index->section == MAIN_MENU_SECTION_FAV) {
        Favorite favorite = fav_at_index(cell_index->row);
        push_window_next_trains(favorite, true);
    } else if (cell_index->section == MAIN_MENU_SECTION_SEARCH) {
        if (cell_index->row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            push_window_search_train(STATION_NON, STATION_NON, true);
#if defined(PBL_PLATFORM_APLITE)
            // Remove main menu window to reduce memory for Aplite.
            window_stack_remove(s_window, false);
#endif
        } else {
            // TODO: Nearby stations
        }
    } else if (cell_index->section == MAIN_MENU_SECTION_SETTING) {
        if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            // Change theme
            settings_set_theme(!settings_is_dark_theme());
#ifdef PBL_COLOR
            ui_setup_theme(s_window, s_menu_layer);
#else
            ui_setup_theme(s_window, s_inverter_layer);
#endif
            
#if !defined(PBL_PLATFORM_APLITE)
            status_bar_set_colors(s_status_bar);
#endif
        } else if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            settings_toggle_locale();
        } else if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_ON_LAUNCH) {
            settings_toggle_is_fav_on_launch();
        }
        menu_layer_reload_data(s_menu_layer);
    }
}

static void menu_layer_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (cell_index->section == MAIN_MENU_SECTION_FAV) {
        ActionListConfig config = (ActionListConfig){
            .num_rows = MAIN_MENU_ACTIONS_COUNT,
            .default_selection = MAIN_MENU_ACTIONS_EDIT,
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
    } else {
        menu_layer_select_callback(s_menu_layer, cell_index, context);
    }
}

// MARK: Window callbacks

static void window_load(Window *window) {
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
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)menu_layer_get_num_sections_callback,
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)menu_layer_get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)menu_layer_get_cell_height_callback,
        .get_header_height = (MenuLayerGetHeaderHeightCallback)menu_layer_get_header_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)menu_layer_draw_row_callback,
        .draw_header = (MenuLayerDrawHeaderCallback)menu_layer_draw_header_callback,
        .select_click = (MenuLayerSelectCallback)menu_layer_select_callback,
        .select_long_click = (MenuLayerSelectCallback)menu_layer_select_long_callback
#if !defined(PBL_PLATFORM_APLITE)
        ,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)menu_layer_draw_background_callback
#endif
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    
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

static void window_appear(Window *window) {
    // BUG FIX:
    // Fixed the wrong menu layer origin when returning to main menu window after adding a favorite
    // Reload the layer to fix it
    menu_layer_reload_data(s_menu_layer);
    
    // Show the selected row
    menu_layer_set_selected_index(s_menu_layer, menu_layer_get_selected_index(s_menu_layer), MenuRowAlignCenter, false);
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
#if !defined(PBL_PLATFORM_APLITE)
    layer_destroy(s_status_bar_background_layer);
    status_bar_layer_destroy(s_status_bar);
#endif
    
#ifdef PBL_BW
    inverter_layer_destroy(s_inverter_layer);
#endif
    window_destroy(s_window);
    s_window = NULL;
}

// MARK: Entry point

void push_window_main_menu(bool animated) {
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, animated);
}

Window *get_window_main_menu() {
    return s_window;
}