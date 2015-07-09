//
//  main_menu_window.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include <localize.h>
#include "main_menu_window.h"
#include "next_trains_window.h"
#include "utilities.h"

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

Window *s_main_window;
static MenuLayer *s_menu_layer;
#ifdef PBL_SDK_3
static StatusBarLayer *s_status_bar;
static Layer *s_battery_layer;
#endif

// Foward declaration

void setup_main_menu_layer_theme();

// Drawing

void draw_main_menu_cell(GContext *ctx, Layer *cell_layer, GColor stroke_color,
                         const char * str_icon, const char * icon_font_key,
                         const char * str_line_1, const char * str_line_2) {
    graphics_context_set_stroke_color(ctx, stroke_color);
    GRect bounds = layer_get_bounds(cell_layer);
    
    // Draw left icon
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT - CELL_ICON_SIZE) / 2,
                             CELL_ICON_SIZE,
                             CELL_ICON_SIZE);
    graphics_draw_round_rect(ctx, frame_icon, 3);
    draw_text(ctx, str_icon, icon_font_key, frame_icon, GTextAlignmentCenter);
    
    // Draw lines
    if (str_line_2 != NULL) {
        GRect frame_line_1 = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                                   -CELL_TEXT_Y_OFFSET + 2, // +2 to get the two lines closer
                                   bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN * 3,
                                   CELL_HEIGHT_2);
        draw_text(ctx, str_line_1, FONT_KEY_GOTHIC_18, frame_line_1, GTextAlignmentLeft);
        
        GRect frame_line_2 = GRect(CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN,
                                   CELL_HEIGHT_2 - CELL_TEXT_Y_OFFSET - 2, // -2 to get the two lines closer
                                   bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN * 3,
                                   CELL_HEIGHT_2);
        draw_text(ctx, str_line_2, FONT_KEY_GOTHIC_18, frame_line_2, GTextAlignmentLeft);
    } else {
        GRect frame_line_1 = GRect(0,
                                   0,
                                   bounds.size.w - CELL_ICON_SIZE - CELL_MARGIN * 3,
                                   CELL_HEIGHT - 4);
        GSize text_size = graphics_text_layout_get_content_size(str_line_1,
                                                                fonts_get_system_font(FONT_KEY_GOTHIC_18),
                                                                frame_line_1,
                                                                GTextOverflowModeTrailingEllipsis,
                                                                GTextAlignmentLeft);
        frame_line_1.origin.y = (frame_line_1.size.h - text_size.h) / 2 - CELL_TEXT_Y_OFFSET;
        frame_line_1.origin.x = CELL_MARGIN + CELL_ICON_SIZE + CELL_MARGIN;
        
        draw_text(ctx, str_line_1, FONT_KEY_GOTHIC_18, frame_line_1, GTextAlignmentLeft);
    }
}

// Menu layer callbacks

static uint16_t get_num_sections_callback(struct MenuLayer *menu_layer, void *context) {
    return MAIN_MENU_SECTION_COUNT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    switch(section_index) {
        case MAIN_MENU_SECTION_FAV:
            return 2;
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
    return HEADER_HEIGHT;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
#ifdef PBL_COLOR
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_highlighted = (menu_index_compare(&selected_index, cell_index) == 0);
    GColor stroke_color = is_highlighted?GColorWhite:curr_fg_color();
#else
    GColor stroke_color = curr_fg_color();
#endif
    int16_t section = cell_index->section;
    int16_t row = cell_index->row;
    if (section == MAIN_MENU_SECTION_FAV ) {
        if (row == 0) {
            draw_main_menu_cell(ctx, cell_layer, stroke_color,
                                "L", FONT_KEY_GOTHIC_14_BOLD,
                                "Paris Saint-Lazare", "Bécon les Bruyères");
        } else if (row == 1) {
            draw_main_menu_cell(ctx, cell_layer, stroke_color,
                                "All", FONT_KEY_GOTHIC_14,
                                "Bibliothèque François Mitterrand", NULL);
        }
    } else if (section == MAIN_MENU_SECTION_SEARCH) {
        if (row == MAIN_MENU_SECTION_SEARCH_ROW_NEARBY) {
            menu_cell_basic_draw(ctx, cell_layer, _("Nearby stations"), _("Based on GPS location"), NULL);
        } else if (row == MAIN_MENU_SECTION_SEARCH_ROW_NAME) {
            menu_cell_basic_draw(ctx, cell_layer, _("A specific station"), _("By choosing letters"), NULL);
        }
    } else if (section == MAIN_MENU_SECTION_SETTING) {
        if (row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            menu_cell_basic_draw(ctx, cell_layer, _("Theme"), get_setting_theme()?_("Dark theme"):_("Light theme"), NULL);
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
        push_next_trains_window();
    } else if (cell_index->section == MAIN_MENU_SECTION_SEARCH) {
        push_next_trains_window();
    } else if (cell_index->section == MAIN_MENU_SECTION_SETTING) {
        if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_THEME) {
            // Change theme
            set_setting_theme(!get_setting_theme());
            setup_main_menu_layer_theme();
        } else if (cell_index->row == MAIN_MENU_SECTION_SETTING_ROW_LANGUAGE) {
            // Change language
            const char* locale_str = setlocale(LC_ALL, NULL);
            char *result;
            if (strncmp(locale_str, "en", 2) == 0) {
                result = setlocale(LC_ALL, "fr_FR");
            } else if (strncmp(locale_str, "fr", 2) == 0) {
                result = setlocale(LC_ALL, "zh_CN");
            } else {
                result = setlocale(LC_ALL, "en_US");
            }
            set_setting_language(result);
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

#ifdef PBL_SDK_3
static void draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context) {
    GRect frame = layer_get_frame(bg_layer);
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
}
#endif

// Window load/unload

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    int16_t status_bar_height = 0;
#ifdef PBL_SDK_3
    status_bar_height = STATUS_BAR_LAYER_HEIGHT;
#endif
    GRect menu_layer_frame = GRect(bounds.origin.x,
                                   bounds.origin.y + status_bar_height,
                                   bounds.size.w,
                                   bounds.size.h - status_bar_height);
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
#ifdef PBL_SDK_3
        ,
        .draw_background = (MenuLayerDrawBackgroundCallback)draw_background_callback
#endif
    });
    setup_main_menu_layer_theme();
    
    // Finally, add status bar
#ifdef PBL_SDK_3
    window_add_status_bar(window_layer, &s_status_bar, &s_battery_layer);
#endif
}

static void window_unload(Window *window) {
    menu_layer_destroy(s_menu_layer);
#ifdef PBL_SDK_3
    layer_destroy(s_battery_layer);
    status_bar_layer_destroy(s_status_bar);
#endif
    window_destroy(window);
    s_main_window = NULL;
}

// Setup UI

void setup_main_menu_layer_theme() {
#ifdef PBL_COLOR
    menu_layer_set_normal_colors(s_menu_layer, curr_bg_color(), curr_fg_color());
    menu_layer_set_highlight_colors(s_menu_layer, GColorCobaltBlue, GColorWhite);
#endif
}

// Entry point

void push_main_menu_window() {
    if(!s_main_window) {
        s_main_window = window_create();
        window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
    }
    window_stack_push(s_main_window, true);
}