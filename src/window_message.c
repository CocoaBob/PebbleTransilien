//
//  window_message.c
//  PebbleTransilien
//
//  Created by CocoaBob on 28/02/16.
//  Copyright © 2016 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

#ifndef PBL_PLATFORM_APLITE

typedef struct {
    Window *window;
    ScrollLayer *scroll_layer;
    TextLayer *text_layer;
    Layer *indicator_up_layer;
    Layer *indicator_down_layer;
    ContentIndicator *indicator_up;
    ContentIndicator *indicator_down;
    char* content;
} Message;

// MARK: Scroll layer callback

static void scroll_layer_content_offset_changed_handler(ScrollLayer *scroll_layer, void *context) {
    Message *user_info = context;
    
    GPoint content_offset = scroll_layer_get_content_offset(scroll_layer);
    GSize content_size = scroll_layer_get_content_size(scroll_layer);
    GRect scroll_layer_bounds = layer_get_bounds(scroll_layer_get_layer(scroll_layer));
    
    content_indicator_set_content_available(user_info->indicator_up, ContentIndicatorDirectionUp, content_offset.y != 0);
    content_indicator_set_content_available(user_info->indicator_down, ContentIndicatorDirectionDown, (content_size.h + content_offset.y) >= scroll_layer_bounds.size.h);
}

// MARK: Add components

static void add_indicators(GRect window_bounds, Message *user_info) {
    user_info->indicator_up_layer = layer_create(GRect(0,
                                                       0,
                                                       window_bounds.size.w,
                                                       STATUS_BAR_LAYER_HEIGHT));
    user_info->indicator_down_layer = layer_create(GRect(0,
                                                         window_bounds.size.h - STATUS_BAR_LAYER_HEIGHT,
                                                         window_bounds.size.w,
                                                         STATUS_BAR_LAYER_HEIGHT));
    layer_add_child(window_get_root_layer(user_info->window), user_info->indicator_up_layer);
    layer_add_child(window_get_root_layer(user_info->window), user_info->indicator_down_layer);
    
    user_info->indicator_up = content_indicator_create();
    const ContentIndicatorConfig up_config = (ContentIndicatorConfig) {
        .layer = user_info->indicator_up_layer,
        .times_out = false,
        .alignment = GAlignCenter,
        .colors = {
            .foreground = GColorBlack,
            .background = GColorChromeYellow
        }
    };
    content_indicator_configure_direction(user_info->indicator_up,
                                          ContentIndicatorDirectionUp,
                                          &up_config);
    
    user_info->indicator_down = content_indicator_create();
    const ContentIndicatorConfig down_config = (ContentIndicatorConfig) {
        .layer = user_info->indicator_down_layer,
        .times_out = false,
        .alignment = GAlignCenter,
        .colors = {
            .foreground = GColorBlack,
            .background = GColorChromeYellow
        }
    };
    content_indicator_configure_direction(user_info->indicator_down,
                                          ContentIndicatorDirectionDown,
                                          &down_config);
    
    content_indicator_set_content_available(user_info->indicator_down, ContentIndicatorDirectionDown, true);
    content_indicator_set_content_available(user_info->indicator_up, ContentIndicatorDirectionUp, false);
}

static void add_text_layer(GRect window_bounds, Message *user_info) {
    GRect text_layer_bounds = GRect(0, 0, window_bounds.size.w - STATUS_BAR_LAYER_HEIGHT - STATUS_BAR_LAYER_HEIGHT, INT16_MAX);
    user_info->text_layer = text_layer_create(text_layer_bounds);
    text_layer_set_font(user_info->text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(user_info->text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(user_info->text_layer, GColorChromeYellow);
    text_layer_set_text_color(user_info->text_layer, GColorBlack);
    
    // Set text and content size
    text_layer_set_text(user_info->text_layer, user_info->content);
    scroll_layer_set_content_size(user_info->scroll_layer, text_layer_get_content_size(user_info->text_layer));
    
    // Add text layer to view hierarchy
    scroll_layer_add_child(user_info->scroll_layer, text_layer_get_layer(user_info->text_layer));
    
    // Must be after added to the view hierarchy
    text_layer_enable_screen_text_flow_and_paging(user_info->text_layer, 4);
}

static void add_scroll_layer(Layer *window_layer, GRect window_bounds, Message *user_info) {
    GRect scroll_layer_bounds = grect_inset(window_bounds, GEdgeInsets(STATUS_BAR_LAYER_HEIGHT));
    user_info->scroll_layer = scroll_layer_create(scroll_layer_bounds);
    scroll_layer_set_context(user_info->scroll_layer, user_info);
    scroll_layer_set_click_config_onto_window(user_info->scroll_layer, user_info->window);
    scroll_layer_set_callbacks(user_info->scroll_layer, (ScrollLayerCallbacks) {
        .content_offset_changed_handler = scroll_layer_content_offset_changed_handler
    });
    layer_add_child(window_layer, scroll_layer_get_layer(user_info->scroll_layer));
}

// MARK: Window callbacks

static void window_load(Window *window) {
    Message *user_info = window_get_user_data(window);
    
    // Window
    window_set_background_color(window, GColorChromeYellow);
    
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_bounds(window_layer);
    
    // Add scroll layer
    add_scroll_layer(window_layer, window_bounds, user_info);
    
    // Add text layer
    add_text_layer(window_bounds, user_info);
    
    // Add indicators
    add_indicators(window_bounds, user_info);
    
    // Enable ScrollLayer paging
    scroll_layer_set_paging(user_info->scroll_layer, true);
}

static void window_unload(Window *window) {
    Message *user_info = window_get_user_data(window);
    
    // Data
    NULL_FREE(user_info->content);

    // Layers
    content_indicator_destroy(user_info->indicator_up);
    content_indicator_destroy(user_info->indicator_down);
    layer_destroy(user_info->indicator_up_layer);
    layer_destroy(user_info->indicator_down_layer);
    text_layer_destroy(user_info->text_layer);
    scroll_layer_destroy(user_info->scroll_layer);
    window_destroy(user_info->window);
    
    NULL_FREE(user_info);
}

// MARK: Entry point

Window* new_window_message(char* content) {
    Message *user_info = calloc(1, sizeof(Message));
    if (user_info) {
        user_info->window = window_create();
        window_set_user_data(user_info->window, user_info);
        window_set_window_handlers(user_info->window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
        });
        
        size_t content_length = strlen(content) + 1;
        user_info->content = calloc(content_length, sizeof(char));
        strncpy(user_info->content, content, content_length);
        
        // Return the window
        return user_info->window;
    }
    return NULL;
}

#endif