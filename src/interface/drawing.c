//
//  drawing.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Basics

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment) {
    graphics_draw_text(ctx, text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

// MARK: Draw header and separators

#ifdef PBL_COLOR
void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(2,
                        TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN * 2,
                        bounds.size.h);
    
    graphics_context_set_text_color(ctx, color);
    draw_text(ctx, title, FONT_KEY_GOTHIC_14_BOLD, frame, GTextAlignmentLeft);
}
#endif

#ifdef PBL_PLATFORM_BASALT
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color) {
    graphics_context_set_stroke_color(ctx, color);
    for (int16_t dx = 0; dx < layer_get_bounds(cell_layer).size.w; dx+=2) {
        graphics_draw_pixel(ctx, GPoint(dx, 0));
    }
}
#endif

// MARK: Draw images

void draw_image_in_rect(GContext* ctx, uint32_t resource_id, GRect rect) {
    GBitmap *bitmap = gbitmap_create_with_resource(resource_id);
#ifdef PBL_COLOR
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
    graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
    gbitmap_destroy(bitmap);
}

// MARK: Draw cells

void draw_centered_title(GContext* ctx, const Layer *cell_layer, const char *title, const char *font_id, GColor color) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(CELL_MARGIN,
                        (bounds.size.h - 20) / 2 + TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN * 2,
                        18);
    graphics_context_set_text_color(ctx, color);
    draw_text(ctx, title, font_id?font_id:FONT_KEY_GOTHIC_18, frame, GTextAlignmentCenter);
}

// MARK: Draw From To Layer, layer height should be 44

void draw_from_to(GContext* ctx,
                        const Layer *layer,
                        DataModelFromTo from_to,
#ifdef PBL_COLOR
                        bool is_highlighed,
#endif
                        GColor text_color) {
    graphics_context_set_text_color(ctx, text_color);
    GRect bounds = layer_get_bounds(layer);
    bool is_from_to = (from_to.to != STATION_NON);
    
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
    stations_get_name(from_to.from, str_from, STATION_NAME_MAX_LENGTH);
    
    GRect frame_line_1 = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                               TEXT_Y_OFFSET + 2, // +2 to get the two lines closer
                               bounds.size.w - FROM_TO_ICON_WIDTH - CELL_MARGIN * 3,
                               CELL_HEIGHT_2);
    if (is_from_to) {
        draw_text(ctx, str_from, FONT_KEY_GOTHIC_18_BOLD, frame_line_1, GTextAlignmentLeft);
        
        GRect frame_line_2 = frame_line_1;
        frame_line_2.origin.y = CELL_HEIGHT_2 + TEXT_Y_OFFSET - 2; // -2 to get the two lines closer
        
        char *str_to = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
        stations_get_name(from_to.to, str_to, STATION_NAME_MAX_LENGTH);
        
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

// MARK: Draw Station layer, layer hight should be 22

void draw_station(GContext *ctx, Layer *cell_layer,
                  GColor text_color,
#ifdef PBL_COLOR
                  bool is_highlighed,
#endif
                  const char * str_time,
                  const char * str_station) {
    graphics_context_set_text_color(ctx, text_color);
    GRect bounds = layer_get_bounds(cell_layer);
    
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT_2 - FROM_TO_ICON_WIDTH) / 2,
                             FROM_TO_ICON_WIDTH,
                             FROM_TO_ICON_WIDTH);
#ifdef PBL_COLOR
    draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
#else
    draw_image_in_rect(ctx, RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
#endif
    
    // Time
    GRect frame_time = GRect(bounds.size.w,
                             TEXT_Y_OFFSET,
                             bounds.size.w,
                             CELL_HEIGHT_2);
    
    if (str_time) {
        GSize time_size = graphics_text_layout_get_content_size(str_time,
                                                                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                                                                frame_time,
                                                                GTextOverflowModeTrailingEllipsis,
                                                                GTextAlignmentRight);
        frame_time.origin.x -= CELL_MARGIN - time_size.w;
        frame_time.size.w = time_size.w;
        draw_text(ctx, str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time, GTextAlignmentRight);
    }
    
    // Station
    GRect frame_station = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                                TEXT_Y_OFFSET,
                                frame_time.origin.x - CELL_MARGIN * 3 - FROM_TO_ICON_WIDTH,
                                CELL_HEIGHT_2);
    draw_text(ctx, str_station, FONT_KEY_GOTHIC_18, frame_station, GTextAlignmentLeft);
}