//
//  text_scroll.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/03/16.
//  Copyright © 2016 CocoaBob. All rights reserved.
//

#include "text_scroll.h"

struct TextScrollData {
    TextScrollConfig config;
    // Indexes
    size_t          curr_index;         // Current scrolling positions
    size_t          max_reset_index;    // Largest reset position
    size_t**        reset_indexes;      // Reset positions, when scrolled to reset positions, return to the beginning
    // Original data
    char **         texts;              // Texts
    GSize*          sizes;              // Text frame sizes
    char **         font_keys;          // Text font keys
    size_t          count;              // Count of texts/indexes
    // Timer
    AppTimer*       timer;
};

static GSize size_of_text(const char *text, const char *font_key, GRect frame) {
    if (frame.size.w <= 0 || frame.size.h <= 0) {
        return GSizeZero;
    }
    return graphics_text_layout_get_content_size(text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

static void text_scroll_timer_callback(TextScrollData *data) {
    if (!data) {
        return;
    }
    data->timer = NULL;
    
    if (data->curr_index >= data->max_reset_index) {
        data->curr_index = 0;
    } else {
        data->curr_index += 1;
    }
    
    if (data->config.update_callback) {
        data->config.update_callback(&(data->config));
    }
    
    // Pause at the beginning or end
    uint32_t timeout_ms = (data->curr_index == 0 || data->curr_index >= data->max_reset_index)?data->config.pause_interval:data->config.frame_interval;
    data->timer = app_timer_register(timeout_ms, (AppTimerCallback)text_scroll_timer_callback, data);
}

void text_scroll_create(TextScrollConfig config, const char ** texts, const GSize ** draw_sizes, const char ** font_keys, const size_t count, TextScrollData **ptr_data) {
    // In case text_scroll_destory() not called before calling text_scroll_create()
    text_scroll_destory(*ptr_data);
    
    TextScrollData *data = calloc(1, sizeof(TextScrollData));
    *ptr_data = data;
    
    data->config = config;
    
    data->count = count;
    data->curr_index = data->max_reset_index = 0;
    
    // Allocate memory for copied texts
    data->texts = calloc(count, sizeof(char *));
    for (size_t i = 0; i < count; ++i) {
        int16_t length = strlen(texts[i]) + 1;
        data->texts[i] = (char *)calloc(length, sizeof(char));
        strncpy(data->texts[i], texts[i], length);
    }
    // Allocate memory for copied sizes
    data->sizes = calloc(count, sizeof(GSize));
    // Allocate memory for copied keys
    data->font_keys = calloc(count, sizeof(char *));
    // Allocate memory for calculated reset_indexes
    data->reset_indexes = calloc(data->count, sizeof(size_t*));
    for (size_t i = 0; i < count; ++i) {
        data->reset_indexes[i] = (size_t *)calloc(1, sizeof(size_t));
    }
    
    // Calculate reset_indexes
    for (size_t i = 0; i < data->count; ++i) {
        GSize draw_size = *((GSize *)draw_sizes[i]);
        memcpy(&data->sizes[i], &draw_size, sizeof(GSize));
        memcpy(&data->font_keys[i], &font_keys[i], sizeof(char **));
        // Find reset offset for the current text in the current draw size
        GRect frame_test = GRect(0, 0, draw_size.w, INT16_MAX);
        GSize text_size = size_of_text((char *)texts[i] + *((size_t*)data->reset_indexes[i]), (char *)font_keys[i], frame_test);
        size_t* ptr_reset_index = data->reset_indexes[i];
        while (*ptr_reset_index < strlen((char *)texts[i]) &&
               (text_size.h > draw_size.h || text_size.w == 0)) {
            ++*ptr_reset_index;
            text_size = size_of_text((char *)texts[i] + *((size_t*)data->reset_indexes[i]), (char *)font_keys[i], frame_test);
        }
        size_t reset_index = *ptr_reset_index;
        // We need the max reset index to sync all the texts
        if (reset_index > data->max_reset_index) {
            data->max_reset_index = reset_index;
        }
    }
    
    // Start timer if necessary
    if (data->max_reset_index > 0) {
        data->timer = app_timer_register(data->config.frame_interval, (AppTimerCallback)text_scroll_timer_callback, data);
    }
}

void text_scroll_destory(TextScrollData *data) {
    if (!data) {
        return;
    }
    
    if (data->timer) {
        app_timer_cancel(data->timer);
        data->timer = NULL;
    }
    
    if (data->reset_indexes) {
        for (size_t i = 0; i < data->count; ++i) {
            free(data->reset_indexes[i]?:NULL);
            data->reset_indexes[i] = NULL;
        }
        free(data->reset_indexes);
        data->reset_indexes = NULL;
    }
    
    if (data->texts) {
        for (size_t i = 0; i < data->count; ++i) {
            free(data->texts[i]?:NULL);
            data->texts[i] = NULL;
        }
        free(data->texts);
        data->texts = NULL;
    }
    
    if (data->sizes) {
        free(data->sizes);
        data->sizes = NULL;
    }
    
    if (data->font_keys) {
        free(data->font_keys);
        data->font_keys = NULL;
    }
    
    free(data);
}

bool text_scroll_is_animating(TextScrollData *data) {
    return (data && data->timer);
}

bool text_scroll_is_unchanged(const char ** new_texts, const GSize ** new_draw_sizes, const char ** new_font_keys, const size_t count, TextScrollData *data) {
    if (!data) {
        return false;
    }
    
    if (count != data->count) {
        return false;
    }
    
    for (size_t i = 0; i < count; ++i) {
        GSize new_size = *((GSize *)new_draw_sizes[i]);
        GSize old_size = data->sizes[i];
        if (new_size.w != old_size.w || new_size.h != old_size.h) {
            return false;
        }
        
        if (new_font_keys[i] != data->font_keys[i]) {
            return false;
        }
    }
    
    for (size_t i = 0; i < count; ++i) {
        const char * new_text = new_texts[i];
        char * old_text = data->texts[i];
        if (strcmp(new_text, old_text) != 0) {
            return false;
        }
    }
    
    return true;
}

char *text_scroll_text(char* text, const size_t index, const bool skip_accent, TextScrollData *data) {
    if (!data || !data->reset_indexes) {
        return text;
    }
    size_t reset_index = *((size_t*)data->reset_indexes[index]);
    char *drawing_text = text;
    if (reset_index < data->curr_index) {
        drawing_text += reset_index;
    } else {
        drawing_text += data->curr_index;
    }
    GRect frame_test = GRect(0, 0, INT16_MAX, data->sizes[index].h);
    GSize text_size = size_of_text(drawing_text, data->font_keys[index], frame_test);
    // In case of accent letters like é/è/à, we have to jump 2 digits
    if (text_size.w == 0) {
        drawing_text += 1;
        if (skip_accent) {
            ++data->curr_index;
        }
    }
    return drawing_text;
}