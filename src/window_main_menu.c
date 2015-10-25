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
    MAIN_MENU_SECTION_SEARCH_ROW_NAME,
    MAIN_MENU_SECTION_SEARCH_ROW_COUNT
};

enum {
    MAIN_MENU_SECTION_SETTING_ROW_THEME,
    MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE,
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

typedef struct {
    Window *window;
    MenuLayer *menu_layer;
#ifdef PBL_BW
    InverterLayer *inverter_layer;
#endif
} MainMenu;

// MARK: Action list callbacks

static char* action_list_get_title_callback(size_t index, MainMenu *user_info) {
    if (index == MAIN_MENU_ACTIONS_MOVE_UP) {
        return _("Move up");
    } else if (index == MAIN_MENU_ACTIONS_EDIT) {
        return _("Edit");
    } else {
        return _("Delete");
    }
}

static bool action_list_is_enabled_callback(size_t index, MainMenu *user_info) {
    if (index == MAIN_MENU_ACTIONS_MOVE_UP) {
        return (fav_get_count() > 1 && menu_layer_get_selected_index(user_info->menu_layer).row > 0);
    }
    return true;
}

static void action_list_select_callback(Window *action_list_window, size_t index, MainMenu *user_info) {
    MenuIndex current_selection = menu_layer_get_selected_index(user_info->menu_layer);
    if (index == MAIN_MENU_ACTIONS_MOVE_UP) {
        fav_move_up_index(current_selection.row);
        menu_layer_set_selected_next(user_info->menu_layer, true, MenuRowAlignCenter, false);
    } else if (index == MAIN_MENU_ACTIONS_DELETE) {
        fav_delete_at_index(current_selection.row);
    } else { // MAIN_MENU_ACTIONS_EDIT
        Favorite favorite = fav_at_index(current_selection.row);
        if (ui_can_push_window()) {
            ui_push_window(new_window_search_train(favorite.from, favorite.to));
        }
    }
}

// MARK: Menu layer callbacks

static uint16_t menu_layer_get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return MAIN_MENU_SECTION_COUNT;
}

static uint16_t menu_layer_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, MainMenu *user_info) {
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

static int16_t menu_layer_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, MainMenu *user_info) {
    return CELL_HEIGHT;
}

static int16_t menu_layer_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, MainMenu *user_info) {
    if (section_index == MAIN_MENU_SECTION_FAV) {
        return fav_get_count()?HEADER_HEIGHT:0;
    }
    return HEADER_HEIGHT;
}

static void menu_layer_draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, MainMenu *user_info) {
    MenuIndex selected_index = menu_layer_get_selected_index(user_info->menu_layer);
#if !defined(PBL_PLATFORM_APLITE) || PBL_COLOR
    bool is_selected = (menu_index_compare(&selected_index, cell_index) == 0);
#endif
#ifdef PBL_COLOR
    bool is_highlighed = settings_is_dark_theme() || is_selected;
    GColor text_color = (is_selected && !settings_is_dark_theme())?curr_bg_color():curr_fg_color();
#endif
    uint16_t section = cell_index->section;
    uint16_t row = cell_index->row;
    if (section == MAIN_MENU_SECTION_FAV ) {
        Favorite favorite = fav_at_index(row);
        draw_from_to(ctx, cell_layer,
#if !defined(PBL_PLATFORM_APLITE)
                     menu_layer_get_layer(user_info->menu_layer), is_selected,
#endif
#ifdef PBL_COLOR
                     is_highlighed,
                     text_color,
#else
                     false,
#endif
                     favorite);
    } else if (section == MAIN_MENU_SECTION_SEARCH) {
        if (row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            menu_cell_basic_draw(ctx, cell_layer, _("Alphabetic..."), _("By choosing letters"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_SETTING) {
        if (row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            menu_cell_basic_draw(ctx, cell_layer, _("Theme"), settings_is_dark_theme()?_("Dark theme"):_("Light theme"), NULL);
        } else if (row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            menu_cell_basic_draw(ctx, cell_layer, "Language", _("English"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_ABOUT) {
        if (row == MAIN_MENU_SECTION_ABOUT_ROW_AUTHOR) {
            menu_cell_basic_draw(ctx, cell_layer, _("Developer"), "@CocoaBob", NULL);
        } else if (row == MAIN_MENU_SECTION_ABOUT_ROW_VERSION) {
            menu_cell_basic_draw(ctx, cell_layer, _("Version"), "1.1", NULL);
        }
    }
}

static void menu_layer_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, MainMenu *user_info) {
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

static void menu_layer_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, MainMenu *user_info) {
    if (cell_index->section == MAIN_MENU_SECTION_FAV) {
        Favorite favorite = fav_at_index(cell_index->row);
        if (ui_can_push_window()) {
            ui_push_window(new_window_next_trains(favorite));
        }
    } else if (cell_index->section == MAIN_MENU_SECTION_SEARCH) {
        if (cell_index->row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            if (ui_can_push_window()) {
                ui_push_window(new_window_search_train(STATION_NON, STATION_NON));
            }
        } else {
            // TODO: Nearby stations
        }
    } else if (cell_index->section == MAIN_MENU_SECTION_SETTING) {
        if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            // Change theme
            settings_set_theme(!settings_is_dark_theme());
#ifdef PBL_COLOR
            ui_setup_theme(user_info->window, user_info->menu_layer);
#else
            ui_setup_theme(user_info->window, user_info->inverter_layer);
#endif
            status_bar_update();
        } else if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            settings_toggle_locale();
        }
        menu_layer_reload_data(user_info->menu_layer);
    }
}

static void menu_layer_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, MainMenu *user_info) {
    if (cell_index->section == MAIN_MENU_SECTION_FAV) {
        ActionListConfig config = (ActionListConfig){
            .context = user_info,
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
        menu_layer_select_callback(user_info->menu_layer, cell_index, user_info);
    }
}

#if !defined(PBL_PLATFORM_APLITE)
static void menu_layer_selection_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) {
    text_scroll_end();
}
#endif

// MARK: Window callbacks

static void window_load(Window *window) {
    MainMenu *user_info = window_get_user_data(window);
    
    Layer *window_layer = window_get_root_layer(user_info->window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    GRect menu_layer_frame = GRect(window_bounds.origin.x,
                                   window_bounds.origin.y + STATUS_BAR_LAYER_HEIGHT,
                                   window_bounds.size.w,
                                   window_bounds.size.h - STATUS_BAR_LAYER_HEIGHT);
    user_info->menu_layer = menu_layer_create(menu_layer_frame);
    layer_add_child(window_layer, menu_layer_get_layer(user_info->menu_layer));
    menu_layer_set_callbacks(user_info->menu_layer, user_info, (MenuLayerCallbacks) {
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
        .selection_changed = (MenuLayerSelectionChangedCallback)menu_layer_selection_changed,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)common_menu_layer_get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)common_menu_layer_draw_separator_callback,
        .draw_background = (MenuLayerDrawBackgroundCallback)common_menu_layer_draw_background_callback
#endif
    });
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(user_info->menu_layer, user_info->window);
    
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

static void window_appear(Window *window) {
    MainMenu *user_info = window_get_user_data(window);
    
    // Add status bar
    ui_setup_status_bar(window_get_root_layer(user_info->window), menu_layer_get_layer(user_info->menu_layer));
    
    // BUG FIX:
    // Fixed the wrong menu layer origin when returning to main menu window after adding a favorite
    // Reload the layer to fix it
    menu_layer_reload_data(user_info->menu_layer);
    
    // Show the selected row
    menu_layer_set_selected_index(user_info->menu_layer, menu_layer_get_selected_index(user_info->menu_layer), MenuRowAlignCenter, false);
    
}

#if !defined(PBL_PLATFORM_APLITE)
static void window_disappear(Window *window) {
    // Stop scrolling text
    text_scroll_end();
}
#endif

static void window_unload(Window *window) {
    MainMenu *user_info = window_get_user_data(window);
    
    menu_layer_destroy(user_info->menu_layer);
#ifdef PBL_BW
    inverter_layer_destroy(user_info->inverter_layer);
#endif
    window_destroy(user_info->window);
    
    free(user_info);
}

// MARK: Entry point

Window* new_window_main_menu() {
    MainMenu *user_info = calloc(1, sizeof(MainMenu));
    if (user_info) {
        user_info->window = window_create();
        window_set_user_data(user_info->window, user_info);
        window_set_window_handlers(user_info->window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
#if !defined(PBL_PLATFORM_APLITE)
            .disappear = window_disappear,
#endif
            .unload = window_unload
        });
        
#ifdef PBL_SDK_2
        // Fullscreen
        window_set_fullscreen(user_info->window, true);
#endif
        
        // Return the window
        return user_info->window;
    }
    return NULL;
}