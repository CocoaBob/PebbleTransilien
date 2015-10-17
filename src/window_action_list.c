//
//  window_action_list.c
//  PebbleTransilien
//
//  Created by CocoaBob on 30/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>

#include "window_action_list.h"

#define ACTION_LIST_BAR_WIDTH 14
#define ACTION_LIST_BAR_POINT_X 7
#define ACTION_LIST_BAR_POINT_Y 10
#define ACTION_LIST_ROW_HEIGHT_MIN 22
#define ACTION_LIST_TITLE_MARGIN 6

static ActionListCallbacks s_callbacks;

static size_t s_num_rows;
static size_t s_default_selection;
static size_t s_row_height;
static GColor s_background_color;
static GColor s_bar_color;
static GColor s_text_color;
#ifdef PBL_COLOR
static GColor s_highlight_text_color;
static GColor s_disabled_text_color;
#endif

static Window *s_window;
static MenuLayer *s_menu_layer;

// MARK: Bar Layer

static void action_list_bar_layer_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    
    graphics_context_set_fill_color(ctx, s_bar_color);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    graphics_context_set_fill_color(ctx, s_background_color);
    graphics_fill_circle(ctx, GPoint(ACTION_LIST_BAR_POINT_X, ACTION_LIST_BAR_POINT_Y), 2);
}

// MARK: Menu layer callbacks

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    return s_num_rows;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    return s_row_height;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
    MenuIndex selected_index = menu_layer_get_selected_index(s_menu_layer);
    bool is_selected = (menu_layer_get_selected_index(s_menu_layer).row == cell_index->row);
    bool is_enabled = true;
    if (s_callbacks.is_enabled && !s_callbacks.is_enabled(cell_index->row)) {
        is_enabled = false;
    }
    GRect bounds = layer_get_bounds(cell_layer);
    
#ifdef PBL_BW
    if (is_selected) {
        // Selected item's background for BW
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, grect_crop(bounds, 4), 4, GCornersAll);
    }
#endif
    
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, is_selected?s_highlight_text_color:(is_enabled?s_text_color:s_disabled_text_color));
#endif
    
    GFont font = fonts_get_system_font(is_enabled?FONT_KEY_GOTHIC_18_BOLD:FONT_KEY_GOTHIC_18);
    
    char *text = s_callbacks.get_title(cell_index->row);
    
    GRect frame = bounds;
    
    frame.origin.x += ACTION_LIST_TITLE_MARGIN;
    frame.size.w -= ACTION_LIST_TITLE_MARGIN << 1;
    
    GSize text_size = graphics_text_layout_get_content_size(text,
                                                            font,
                                                            frame,
                                                            GTextOverflowModeTrailingEllipsis,
                                                            GTextAlignmentLeft);
    
    frame.origin.y = (bounds.size.h - text_size.h) / 2;
    frame.size.h = text_size.h;
    
    graphics_draw_text(ctx, text, font, frame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    if (!s_callbacks.is_enabled || s_callbacks.is_enabled(cell_index->row)) {
        s_callbacks.select_click(s_window, cell_index->row);
    }
}

static int16_t get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 0;
}

// MARK: Window callbacks

static void window_load(Window *window) {
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    GRect menu_layer_frame = GRect(window_bounds.origin.x  + ACTION_LIST_BAR_WIDTH,
                                   window_bounds.origin.y,
                                   window_bounds.size.w - ACTION_LIST_BAR_WIDTH,
                                   window_bounds.size.h);
    s_menu_layer = menu_layer_create(menu_layer_frame);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
    
    // Setup menu layer
#if !defined(PBL_PLATFORM_APLITE)
    menu_layer_pad_bottom_enable(s_menu_layer, false);
#endif
    
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
        .select_click = (MenuLayerSelectCallback)select_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback
    });
    
#ifdef PBL_COLOR
    menu_layer_set_normal_colors(s_menu_layer, s_background_color, s_text_color);
    menu_layer_set_highlight_colors(s_menu_layer, s_background_color, s_highlight_text_color);
#else
    window_set_background_color(window, s_background_color);
#endif
    
    menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, s_default_selection), MenuRowAlignNone, false);
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(s_menu_layer, window);
    
    // Add bar layer
    GRect bar_layer_frame = GRect(window_bounds.origin.x,
                                  window_bounds.origin.y,
                                  ACTION_LIST_BAR_WIDTH,
                                  window_bounds.size.h);
    Layer *bar_layer = layer_create(bar_layer_frame);
    layer_set_update_proc(bar_layer, action_list_bar_layer_proc);
    layer_add_child(window_layer, bar_layer);
    
    // Calculate row height
    if (s_num_rows > 0) {
        Layer *window_layer = window_get_root_layer(s_window);
        GRect window_bounds = layer_get_bounds(window_layer);
        s_row_height = window_bounds.size.h / s_num_rows;
        if (s_row_height < ACTION_LIST_ROW_HEIGHT_MIN) {
            s_row_height = ACTION_LIST_ROW_HEIGHT_MIN;
        }
    }
}

static void window_unload(Window *window) {
    // Window
    menu_layer_destroy(s_menu_layer);
    window_destroy(s_window);
    s_window = NULL;
}

static void window_appear(Window *window) {
#ifdef PBL_BW
    // BUG FIX: When it appears for the 1st time, the selected row's inverter layer position is incorrect
    menu_layer_reload_data(s_menu_layer);
#endif
    //    printf("Heap Total <%4dB> Used <%4dB> Free <%4dB>",heap_bytes_used()+heap_bytes_free(),heap_bytes_used(),heap_bytes_free());
}

// MARK: Entry point

void action_list_present_with_callbacks(ActionListCallbacks callbacks) {
    // Prepare window
    if(!s_window) {
        s_window = window_create();
        window_set_window_handlers(s_window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .unload = window_unload
        });
    }
    
    // Callbacks
    s_callbacks = callbacks;
    
    if (s_callbacks.get_num_rows) {
        s_num_rows = s_callbacks.get_num_rows();
    } else {
        s_num_rows = 0;
    }
    
    if (s_callbacks.get_default_selection) {
        s_default_selection = s_callbacks.get_default_selection();
    } else {
        s_default_selection = 0;
    }
    
    if (s_callbacks.get_background_color) {
        s_background_color = s_callbacks.get_background_color();
    } else {
        s_background_color = GColorBlack;
    }
    
#ifdef PBL_COLOR
    if (s_callbacks.get_text_color) {
        s_text_color = s_callbacks.get_text_color();
    } else {
        s_text_color = GColorLightGray;
    }
#else
    s_text_color = GColorBlack;
#endif
    
#ifdef PBL_COLOR
    if (s_callbacks.get_highlight_text_color) {
        s_highlight_text_color = s_callbacks.get_highlight_text_color();
    } else {
        s_highlight_text_color = GColorWhite;
    }
    
    if (s_callbacks.get_disabled_text_color) {
        s_disabled_text_color = s_callbacks.get_disabled_text_color();
    } else {
        s_disabled_text_color = GColorDarkGray;
    }
#endif
    
    if (s_callbacks.get_bar_color) {
        s_bar_color = s_callbacks.get_bar_color();
    } else {
        s_bar_color = GColorWhite;
    }
    
    // Push the window
    window_stack_push(s_window, true);
}