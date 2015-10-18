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
#define ACTION_LIST_TEXT_MARGIN 6
#define ACTION_LIST_TEXT_Y_OFFSET -2
#ifdef PBL_BW
#define ACTION_LIST_SELECTION_MARGIN 4
#endif

typedef struct {
    ActionListConfig *config;
    Window *window;
    MenuLayer *menu_layer;
    size_t row_height;
    Layer *bar_layer;
} ActionList;

// MARK: Bar Layer

static void action_list_bar_layer_proc(Layer *layer, GContext *ctx) {
    ActionList *action_list = *((ActionList**)layer_get_data(layer));
    
    GRect bounds = layer_get_bounds(layer);
    
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, action_list->config->colors.background);
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, action_list->config->colors.foreground);
#else
    graphics_context_set_fill_color(ctx, GColorBlack);
#endif
    graphics_fill_circle(ctx, GPoint(ACTION_LIST_BAR_POINT_X, ACTION_LIST_BAR_POINT_Y), 2);
}

// MARK: Menu layer callbacks

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
    ActionList *action_list = context;
    return action_list->config->num_rows;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    ActionList *action_list = context;
    return action_list->row_height;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *context) {
    ActionList *action_list = context;
    
    bool is_selected = (menu_layer_get_selected_index(action_list->menu_layer).row == cell_index->row);
    bool is_enabled = true;
    if (action_list->config->callbacks.is_enabled && !action_list->config->callbacks.is_enabled(cell_index->row)) {
        is_enabled = false;
    }
    GRect bounds = layer_get_bounds(cell_layer);
    
#ifdef PBL_BW
    // Selected item's background for BW
    if (is_selected) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
        
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, grect_crop(bounds, ACTION_LIST_SELECTION_MARGIN), 4, GCornersAll);
    }
#endif
    
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, is_selected?action_list->config->colors.text_selected:(is_enabled?action_list->config->colors.text:action_list->config->colors.text_disabled));
#endif
    
    GFont font = fonts_get_system_font(is_enabled?FONT_KEY_GOTHIC_18_BOLD:FONT_KEY_GOTHIC_18);
    char *text = action_list->config->callbacks.get_title(cell_index->row);
    GRect text_frame = grect_crop(bounds,
#ifdef PBL_BW
                                  ACTION_LIST_SELECTION_MARGIN +
#endif
                                  ACTION_LIST_TEXT_MARGIN);
    GSize text_size = graphics_text_layout_get_content_size(text, font, text_frame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    text_frame.origin.y += (text_frame.size.h - text_size.h) / 2;
    text_frame.size.h = text_size.h;
    
    text_frame.origin.y += ACTION_LIST_TEXT_Y_OFFSET;
    
    graphics_draw_text(ctx, text, font, text_frame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
    ActionList *action_list = context;
    if (!action_list->config->callbacks.is_enabled || action_list->config->callbacks.is_enabled(cell_index->row)) {
        action_list->config->callbacks.select_click(action_list->window, cell_index->row);
    }
}

static int16_t get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 0;
}

// MARK: Window callbacks

static void window_load(Window *window) {
    ActionList *action_list = window_get_user_data(window);
    
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    action_list->menu_layer = menu_layer_create(GRect(bounds.origin.x  + ACTION_LIST_BAR_WIDTH,
                                                      bounds.origin.y,
                                                      bounds.size.w - ACTION_LIST_BAR_WIDTH,
                                                      bounds.size.h));
    layer_add_child(window_layer, menu_layer_get_layer(action_list->menu_layer));
    
    // Setup menu layer
#if !defined(PBL_PLATFORM_APLITE)
    menu_layer_pad_bottom_enable(action_list->menu_layer, false);
#endif
    
    menu_layer_set_callbacks(action_list->menu_layer, action_list, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
        .select_click = (MenuLayerSelectCallback)select_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback
    });
    
#ifdef PBL_COLOR
    menu_layer_set_normal_colors(action_list->menu_layer, GColorBlack, action_list->config->colors.text);
    menu_layer_set_highlight_colors(action_list->menu_layer, GColorBlack, action_list->config->colors.text_selected);
#else
    window_set_background_color(window, GColorBlack);
#endif
    
    menu_layer_set_selected_index(action_list->menu_layer, MenuIndex(0, action_list->config->default_selection), MenuRowAlignNone, false);
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(action_list->menu_layer, window);
    
    // Add bar layer
    GRect bar_layer_frame = GRect(bounds.origin.x,
                                  bounds.origin.y,
                                  ACTION_LIST_BAR_WIDTH,
                                  bounds.size.h);
    action_list->bar_layer = layer_create_with_data(bar_layer_frame, sizeof(ActionList **));
    *((ActionList **)layer_get_data(action_list->bar_layer)) = action_list;
    layer_set_update_proc(action_list->bar_layer, action_list_bar_layer_proc);
    layer_add_child(window_layer, action_list->bar_layer);
    
    // Calculate row height
    if (action_list->config->num_rows > 0) {
        action_list->row_height = bounds.size.h / action_list->config->num_rows;
        if (action_list->row_height < ACTION_LIST_ROW_HEIGHT_MIN) {
            action_list->row_height = ACTION_LIST_ROW_HEIGHT_MIN;
        }
    }
}

static void window_unload(Window *window) {
    ActionList *action_list = window_get_user_data(window);
    
    menu_layer_destroy(action_list->menu_layer);
    layer_destroy(action_list->bar_layer);
    window_destroy(action_list->window);
    
    free(action_list->config);
    free(action_list);
}

static void window_appear(Window *window) {
#ifdef PBL_SDK_2
    ActionList *action_list = window_get_user_data(window);
    // BUG FIX: When it appears for the 1st time, the selected row's inverter layer position is incorrect
    menu_layer_reload_data(action_list->menu_layer);
#endif
}

// MARK: Entry point

void action_list_open(ActionListConfig *config) {
    if (!config) {
        return;
    }
    
    ActionList *action_list = malloc(sizeof(ActionList));
    if (action_list) {
        memset(action_list, 0, sizeof(ActionList));
        action_list->config = malloc(sizeof(ActionListConfig));
        memcpy(action_list->config, config, sizeof(ActionListConfig));
        
        action_list->window = window_create();
        window_set_user_data(action_list->window, action_list);
        window_set_window_handlers(action_list->window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .unload = window_unload
        });
        
#ifdef PBL_SDK_2
        window_set_fullscreen(action_list->window, true);
#endif
    }
    
    // Push the window
    window_stack_push(action_list->window, true);
}