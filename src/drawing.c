//
//  drawing.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment) {
    graphics_draw_text(ctx, text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

// Draw header and separators

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(2,
                        -CELL_TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN * 2,
                        bounds.size.h);
    
    draw_text(ctx, title, FONT_KEY_GOTHIC_14_BOLD, frame, GTextAlignmentLeft);
}

void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color) {
    graphics_context_set_stroke_color(ctx, color);
    for (int16_t dx = 0; dx < layer_get_bounds(cell_layer).size.w; dx+=2) {
        graphics_draw_pixel(ctx, GPoint(dx, 0));
    }
}

// Draw images

void draw_image_in_rect(GContext* ctx, uint32_t resource_id, GRect rect) {
    GBitmap *bitmap = gbitmap_create_with_resource(resource_id);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
    gbitmap_destroy(bitmap);
}