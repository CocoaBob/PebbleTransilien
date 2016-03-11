//
//  text_scroll.h
//  PebbleTransilien
//
//  Created by CocoaBob on 09/03/16.
//  Copyright © 2016 CocoaBob. All rights reserved.
//

#pragma once

#include <pebble.h>

typedef void (*TextScrollUpdateCb)(void *context); // Context is TextScrollConfig

typedef struct {
    int16_t frame_interval;            // Time interval between each frame, default 100ms
    int16_t pause_interval;             // Time interval for the beginning or end of the texts, default 1000ms
    TextScrollUpdateCb update_callback; // Callback function to update display
    void* user_context;
} TextScrollConfig;

struct TextScrollData;
typedef struct TextScrollData TextScrollData;

//! Calculate offset indexes and begin texts scrolling timer,
//! @param config TextScrollConfig.
//! @param texts_pointers The pointers of texts to be scrolled.
//! @param draw_sizes The sizes of texts to be scrolled.
//! @param font_keys The font keys of texts to be scrolled.
//! @param count The count of texts.
//! @param ptr_data A pointer to TextScrollData, to get the data back.
//! @note Texts and frames are assumed to be unchanged during the scrolling.
void text_scroll_create(TextScrollConfig config, const char ** texts, const GSize ** draw_sizes, const char ** font_keys, const size_t count, TextScrollData **ptr_data);

//! End the texts scrolling
//! @param data TextScrollData.
void text_scroll_destory(TextScrollData *data);

//! Check if texts are scrolling
//! @param data TextScrollData.
bool text_scroll_is_animating(TextScrollData *data);

//! Check if texts draw sizes are the same
//! @param draw_sizes The sizes of texts to be scrolled.
bool text_scroll_is_unchanged(const char ** new_texts, const GSize ** new_draw_sizes, const char ** new_font_keys, const size_t count, TextScrollData *data);

//! Get the current text to display based on the current scrolling position
//! @param text The pointer points to the beginning of the test
//! @param index The index of the text in the given texts list for text_scroll_create()
//! @param skip_accent As accent letter occupies 2 bytes, we need to skip 1 byte
//! @param data TextScrollData.
char *text_scroll_text(char* text, const size_t index, const bool skip_accent, TextScrollData *data);