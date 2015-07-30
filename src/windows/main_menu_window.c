//
//  main_menu_window.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

enum {
    MAIN_MENU_SECTION_FAV = 0,
    MAIN_MENU_SECTION_SEARCH,
    MAIN_MENU_SECTION_SETTING,
    MAIN_MENU_SECTION_ABOUT,
    MAIN_MENU_SECTION_COUNT
};

enum {
    MAIN_MENU_SECTION_SEARCH_ROW_NEARBY = 0,
    MAIN_MENU_SECTION_SEARCH_ROW_NAME,
    MAIN_MENU_SECTION_SEARCH_ROW_COUNT
};

enum {
    MAIN_MENU_SECTION_SETTING_ROW_THEME = 0,
    MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE,
    MAIN_MENU_SECTION_SETTING_ROW_COUNT
};

enum {
    MAIN_MENU_SECTION_ABOUT_ROW_AUTHOR = 0,
    MAIN_MENU_SECTION_ABOUT_ROW_VERSION,
    MAIN_MENU_SECTION_ABOUT_ROW_COUNT
};

static Window *s_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_PLATFORM_BASALT
static StatusBarLayer *s_status_bar;
static Layer *s_status_bar_background_layer;
static Layer *s_status_bar_overlay_layer;
#endif

#ifdef PBL_BW
static InverterLayer *s_inverter_layer;
#endif

// MARK: Constants

// MARK: Drawing

static void draw_menu_layer_cell(GContext *ctx, Layer *cell_layer,
                                 GColor text_color,
#ifdef PBL_COLOR
                                 bool is_highlighed,
#endif
                                 StationIndex idx_from, StationIndex idx_to) {
    graphics_context_set_text_color(ctx, text_color);
    GRect bounds = layer_get_bounds(cell_layer);
    bool is_from_to = (idx_to != STATION_NON);
    
    // Draw left icon
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT - FROM_TO_ICON_HEIGHT) / 2,
                             FROM_TO_ICON_WIDTH,
                             is_from_to?FROM_TO_ICON_HEIGHT:FROM_TO_ICON_WIDTH);
#ifdef PBL_COLOR
    if (is_from_to) {
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_TO_DARK:RESOURCE_ID_IMG_FROM_TO_LIGHT, frame_icon);
    } else {
        draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    }
#else
    if (is_from_to) {
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_FROM_TO_LIGHT, frame_icon);
    } else {
        draw_image_in_rect(ctx, RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    }
#endif
    
    // Draw lines
    char *str_from = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    stations_get_name(idx_from, str_from, STATION_NAME_MAX_LENGTH);

    GRect frame_line_1 = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                               TEXT_Y_OFFSET + 2, // +2 to get the two lines closer
                               bounds.size.w - FROM_TO_ICON_WIDTH - CELL_MARGIN * 3,
                               CELL_HEIGHT_2);
    if (is_from_to) {
        draw_text(ctx, str_from, FONT_KEY_GOTHIC_18_BOLD, frame_line_1, GTextAlignmentLeft);
        
        GRect frame_line_2 = frame_line_1;
        frame_line_2.origin.y = CELL_HEIGHT_2 + TEXT_Y_OFFSET - 2; // -2 to get the two lines closer
        
        char *str_to = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(idx_to, str_to, STATION_NAME_MAX_LENGTH);

        draw_text(ctx, str_to, FONT_KEY_GOTHIC_18_BOLD, frame_line_2, GTextAlignmentLeft);
        
        free(str_to);
    } else {
        frame_line_1.size.h = bounds.size.h;
        GSize text_size = graphics_text_layout_get_content_size(str_from,
                                                                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                                                                frame_line_1,
                                                                GTextOverflowModeTrailingEllipsis,
                                                                GTextAlignmentLeft);
        frame_line_1.size.h = text_size.h;
        
        draw_text(ctx, str_from, FONT_KEY_GOTHIC_18_BOLD, frame_line_1, GTextAlignmentLeft);
    }
    
    free(str_from);
}

// MARK: Menu layer callbacks

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return MAIN_MENU_SECTION_COUNT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    switch(section_index) {
        case MAIN_MENU_SECTION_FAV:
            return fav_get_count();
        case MAIN_MENU_SECTION_SEARCH:
            return MAIN_MENU_SECTION_SEARCH_ROW_COUNT;
        case MAIN_MENU_SECTION_SETTING:
            return MAIN_MENU_SECTION_SETTING_ROW_COUNT;
        case MAIN_MENU_SECTION_ABOUT:
            return MAIN_MENU_SECTION_ABOUT_ROW_COUNT;
        default:
            return 0;
    }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return CELL_HEIGHT;
}

static int16_t get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
    if (section_index == MAIN_MENU_SECTION_FAV) {
        return fav_get_count()?HEADER_HEIGHT:0;
    }
    return HEADER_HEIGHT;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
    bool is_dark_theme = status_is_dark_theme();
#ifdef PBL_COLOR
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (menu_index_compare(&selected_index, cell_index) == 0);
    bool is_highlighed = is_dark_theme || is_selected;
    GColor text_color = (is_selected && !is_dark_theme)?curr_bg_color():curr_fg_color();
#else
    GColor text_color = curr_fg_color();
#endif
    uint16_t section = cell_index->section;
    uint16_t row = cell_index->row;
    if (section == MAIN_MENU_SECTION_FAV ) {
        Favorite favorite = fav_at_index(row);
        draw_menu_layer_cell(ctx, cell_layer,
                             text_color,
#ifdef PBL_COLOR
                             is_highlighed,
#endif
                             favorite.from, favorite.to);
    } else if (section == MAIN_MENU_SECTION_SEARCH) {
        if (row == MAIN_MENU_SECTION_SEARCH_ROW_NEARBY) {
            menu_cell_basic_draw(ctx, cell_layer, "Nearby...", _("Based on location"), NULL);
        } else if (row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            menu_cell_basic_draw(ctx, cell_layer, "Alphabetic...", _("By choosing letters"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_SETTING) {
        if (row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            menu_cell_basic_draw(ctx, cell_layer, _("Theme"), is_dark_theme?_("Dark theme"):_("Light theme"), NULL);
        } else if (row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            menu_cell_basic_draw(ctx, cell_layer, "Language", _("English"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_ABOUT) {
        if (row == MAIN_MENU_SECTION_ABOUT_ROW_AUTHOR) {
            menu_cell_basic_draw(ctx, cell_layer, _("Developer"), "@CocoaBob", NULL);
        } else if (row == MAIN_MENU_SECTION_ABOUT_ROW_VERSION) {
            menu_cell_basic_draw(ctx, cell_layer, _("Version"), "1.0.0", NULL);
        }
    }
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
    if (section_index == MAIN_MENU_SECTION_FAV) {
        draw_menu_header(ctx, cell_layer, _("Favorites"), curr_fg_color());
    } else if (section_index == MAIN_MENU_SECTION_SEARCH) {
        draw_menu_header(ctx, cell_layer, _("Search"), curr_fg_color());
    } else if (section_index == MAIN_MENU_SECTION_SETTING) {
        draw_menu_header(ctx, cell_layer, _("Settings"), curr_fg_color());
    } else if (section_index == MAIN_MENU_SECTION_ABOUT) {
        draw_menu_header(ctx, cell_layer, _("About"), curr_fg_color());
    }
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (cell_index->section == MAIN_MENU_SECTION_FAV) {
        Favorite favorite = fav_at_index(cell_index->row);
        push_next_trains_window(favorite);
    } else if (cell_index->section == MAIN_MENU_SECTION_SEARCH) {
        if (cell_index->row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            push_search_train_window();
        } else {
            // TODO: Nearby stations
        }
    } else if (cell_index->section == MAIN_MENU_SECTION_SETTING) {
        if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            // Change theme
            storage_set_theme(!status_is_dark_theme());
#ifdef PBL_COLOR
            setup_ui_theme(s_window, s_menu_layer);
#else
            setup_ui_theme(s_window, s_inverter_layer);
#endif
            
#ifdef PBL_PLATFORM_BASALT
            status_bar_set_colors(s_status_bar);
#endif
        } else if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            // Change language
            const char* locale_str = setlocale(LC_ALL, NULL);
            char *result;
            if (strncmp(locale_str, "en", 2) == 0) {
                result = setlocale(LC_ALL, "fr_FR");
            } else {
                result = setlocale(LC_ALL, "en_US");
            }
            storage_set_locale(result);
            locale_init();
        }
        menu_layer_reload_data(menu_layer);
    }
}

static void select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (cell_index->section == MAIN_MENU_SECTION_FAV) {
        // Delete selected favorite
    }
}

static int16_t get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return SEPARATOR_HEIGHT;
}

static void draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
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
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Load favorites
    load_favorites();
    
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
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)get_num_sections_callback,
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
        .get_header_height = (MenuLayerGetHeaderHeightCallback)get_header_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
        .draw_header = (MenuLayerDrawHeaderCallback)draw_header_callback,
        .select_click = (MenuLayerSelectCallback)select_callback,
        .select_long_click = (MenuLayerSelectCallback)select_long_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback,
        .draw_separator = (MenuLayerDrawSeparatorCallback)draw_separator_callback
#ifdef PBL_PLATFORM_BASALT
        ,
        .draw_background = (MenuLayerDrawBackgroundCallback)draw_background_callback
#endif
    });
    
    // Finally, add status bar
#ifdef PBL_PLATFORM_BASALT
    window_add_status_bar(window_layer, &s_status_bar, &s_status_bar_background_layer, &s_status_bar_overlay_layer);
#endif
    
#ifdef PBL_BW
    s_inverter_layer = inverter_layer_create(window_bounds);
#endif
    
#ifdef PBL_COLOR
    setup_ui_theme(s_window, s_menu_layer);
#else
    setup_ui_theme(s_window, s_inverter_layer);
#endif
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
    unload_favorites();
#ifdef PBL_PLATFORM_BASALT
    layer_destroy(s_status_bar_background_layer);
    layer_destroy(s_status_bar_overlay_layer);
    status_bar_layer_destroy(s_status_bar);
#endif
    
#ifdef PBL_BW
    inverter_layer_destroy(s_inverter_layer);
#endif
    window_destroy(s_window);
    s_window = NULL;
}

// MARK: Entry point

void push_main_menu_window() {
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_window, true);
}