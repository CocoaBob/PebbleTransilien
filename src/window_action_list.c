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
    GRect bounds = layer_get_bounds(layer);
    
#ifdef PBL_COLOR
    ActionList *user_info = *((ActionList**)layer_get_data(layer));
    graphics_context_set_fill_color(ctx, user_info->config->colors.background);
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, user_info->config->colors.foreground);
#else
    graphics_context_set_fill_color(ctx, GColorBlack);
#endif
    graphics_fill_circle(ctx, GPoint(ACTION_LIST_BAR_POINT_X, ACTION_LIST_BAR_POINT_Y), 2);
}

// MARK: Menu layer callbacks

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, ActionList *user_info) {
    return user_info->config->num_rows;
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, ActionList *user_info) {
    return user_info->row_height;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, ActionList *user_info) {
    bool is_selected = (menu_layer_get_selected_index(user_info->menu_layer).row == cell_index->row);
    bool is_enabled = true;
    if (user_info->config->callbacks.is_enabled && !user_info->config->callbacks.is_enabled(cell_index->row, user_info->config->context)) {
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
    graphics_context_set_text_color(ctx, is_selected?user_info->config->colors.text_selected:(is_enabled?user_info->config->colors.text:user_info->config->colors.text_disabled));
#endif
    
    GFont font = fonts_get_system_font(is_enabled?FONT_KEY_GOTHIC_18_BOLD:FONT_KEY_GOTHIC_18);
    char *text = user_info->config->callbacks.get_title(cell_index->row, user_info->config->context);
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

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, ActionList *user_info) {
    if (!user_info->config->callbacks.is_enabled || user_info->config->callbacks.is_enabled(cell_index->row, user_info->config->context)) {
        user_info->config->callbacks.select_click(user_info->window, cell_index->row, user_info->config->context);
        // Must remove the window after using it's user_info, otherwise user_info will be released too early.
        window_stack_remove(user_info->window, true);
    }
}

static int16_t get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 0;
}

// MARK: Window callbacks

static void window_load(Window *window) {
    ActionList *user_info = window_get_user_data(window);
    
    // Window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    
    // Add menu layer
    user_info->menu_layer = menu_layer_create(GRect(bounds.origin.x  + ACTION_LIST_BAR_WIDTH,
                                                      bounds.origin.y,
                                                      bounds.size.w - ACTION_LIST_BAR_WIDTH,
                                                      bounds.size.h));
    layer_add_child(window_layer, menu_layer_get_layer(user_info->menu_layer));
    
    // Setup menu layer
#if !defined(PBL_PLATFORM_APLITE)
    menu_layer_pad_bottom_enable(user_info->menu_layer, false);
#endif
    
    menu_layer_set_callbacks(user_info->menu_layer, user_info, (MenuLayerCallbacks) {
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
        .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
        .select_click = (MenuLayerSelectCallback)select_callback,
        .get_separator_height = (MenuLayerGetSeparatorHeightCallback)get_separator_height_callback
    });
    
#ifdef PBL_COLOR
    menu_layer_set_normal_colors(user_info->menu_layer, GColorBlack, user_info->config->colors.text);
    menu_layer_set_highlight_colors(user_info->menu_layer, GColorBlack, user_info->config->colors.text_selected);
#else
    window_set_background_color(window, GColorBlack);
#endif
    
    // Setup Click Config Providers
    menu_layer_set_click_config_onto_window(user_info->menu_layer, window);
    
    // Add bar layer
    GRect bar_layer_frame = GRect(bounds.origin.x,
                                  bounds.origin.y,
                                  ACTION_LIST_BAR_WIDTH,
                                  bounds.size.h);
    user_info->bar_layer = layer_create_with_data(bar_layer_frame, sizeof(ActionList **));
    *((ActionList **)layer_get_data(user_info->bar_layer)) = user_info;
    layer_set_update_proc(user_info->bar_layer, action_list_bar_layer_proc);
    layer_add_child(window_layer, user_info->bar_layer);
    
    // Calculate row height
    if (user_info->config->num_rows > 0) {
        user_info->row_height = bounds.size.h / user_info->config->num_rows;
        if (user_info->row_height < ACTION_LIST_ROW_HEIGHT_MIN) {
            user_info->row_height = ACTION_LIST_ROW_HEIGHT_MIN;
        }
    }
}

static void window_unload(Window *window) {
    ActionList *user_info = window_get_user_data(window);
    
    menu_layer_destroy(user_info->menu_layer);
    layer_destroy(user_info->bar_layer);
    window_destroy(user_info->window);
    
    free(user_info->config);
    free(user_info);
}

static void window_appear(Window *window) {
    ActionList *user_info = window_get_user_data(window);
    
    menu_layer_set_selected_index(user_info->menu_layer, MenuIndex(0, user_info->config->default_selection), MenuRowAlignNone, false);
//    printf("Heap Total <%4dB> Used <%4dB> Free <%4dB>",heap_bytes_used()+heap_bytes_free(),heap_bytes_used(),heap_bytes_free());
}

// MARK: Entry point

void action_list_open(ActionListConfig *config) {
    if (!config) {
        return;
    }
    
    ActionList *user_info = calloc(1, sizeof(ActionList));
    if (user_info) {
        user_info->config = malloc(sizeof(ActionListConfig));
        memcpy(user_info->config, config, sizeof(ActionListConfig));
        
        user_info->window = window_create();
        window_set_user_data(user_info->window, user_info);
        window_set_window_handlers(user_info->window, (WindowHandlers) {
            .load = window_load,
            .appear = window_appear,
            .unload = window_unload
        });
        
#ifdef PBL_SDK_2
        // Fullscreen
        window_set_fullscreen(user_info->window, true);
#endif
        
        window_stack_push(user_info->window, true);
    }
}