//
//  ui_common.c
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Darw Basics

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment) {
    graphics_draw_text(ctx, text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

// MARK: Draw header and separators

#ifdef PBL_COLOR
void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(2,
                        TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN_2,
                        bounds.size.h);
    
    graphics_context_set_text_color(ctx, color);
    draw_text(ctx, title, FONT_KEY_GOTHIC_14_BOLD, frame, GTextAlignmentLeft);
}
#endif

#if !defined(PBL_PLATFORM_APLITE)
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color) {
    graphics_context_set_stroke_color(ctx, color);
    for (int16_t dx = 0; dx < layer_get_bounds(cell_layer).size.w; dx+=2) {
        graphics_draw_pixel(ctx, GPoint(dx, 0));
    }
}
#endif

// MARK: Draw images

void draw_image_in_rect(GContext* ctx,
#ifdef PBL_BW
                        bool is_inverted,
#endif
                        uint32_t resource_id,
                        GRect rect) {
    GBitmap *bitmap = gbitmap_create_with_resource(resource_id);
#ifdef PBL_COLOR
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
    graphics_context_set_compositing_mode(ctx, is_inverted?GCompOpAssignInverted:GCompOpAssign);
#endif
    graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
    gbitmap_destroy(bitmap);
}

// MARK: Draw cells

void draw_centered_title(GContext* ctx,
                         const Layer *cell_layer,
                         bool is_inverted,
                         const char *title,
                         const char *font_id) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(CELL_MARGIN,
                        (bounds.size.h - 20) / 2 + TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN_2,
                        18);
#ifdef PBL_COLOR
    if (is_inverted) {
        graphics_context_set_fill_color(ctx, curr_bg_color());
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }
    graphics_context_set_text_color(ctx, curr_fg_color());
#else
    graphics_context_set_fill_color(ctx, is_inverted?curr_fg_color():curr_bg_color());
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, is_inverted?curr_bg_color():curr_fg_color());
#endif
    draw_text(ctx, title, font_id?font_id:FONT_KEY_GOTHIC_18_BOLD, frame, GTextAlignmentCenter);
}

// MARK: Draw From To Layer, layer height should be 44

void draw_from_to(GContext* ctx,
                  const Layer *layer,
#ifdef PBL_COLOR
                  bool is_highlighed,
                  GColor text_color,
#else
                  bool is_inverted,
#endif
                  DataModelFromTo from_to) {
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, text_color);
#else
    graphics_context_set_text_color(ctx, is_inverted?GColorWhite:GColorBlack);
#endif
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
        draw_image_in_rect(ctx, is_inverted, RESOURCE_ID_IMG_FROM_TO_LIGHT, frame_icon);
    } else {
        draw_image_in_rect(ctx, is_inverted, RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    }
#endif
    
    // Draw lines
    char *str_from = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    stations_get_name(from_to.from, str_from, STATION_NAME_MAX_LENGTH);
    
    GRect frame_line_1 = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                               TEXT_Y_OFFSET + 2, // +2 to get the two lines closer
                               bounds.size.w - FROM_TO_ICON_WIDTH - CELL_MARGIN_3,
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
                  MenuLayer *menu_layer, bool is_selected,
#ifdef PBL_COLOR
                  GColor text_color,
                  bool is_highlighed,
#else
                  bool is_inverted,
#endif
                  char * str_time,
                  char * str_station) {
    GRect bounds = layer_get_bounds(cell_layer);
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, text_color);
#else
    if (is_inverted) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }
    graphics_context_set_text_color(ctx, is_inverted?GColorWhite:GColorBlack);
#endif
    
    // Icon
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT_2 - FROM_TO_ICON_WIDTH) / 2,
                             FROM_TO_ICON_WIDTH,
                             FROM_TO_ICON_WIDTH);
#ifdef PBL_COLOR
    draw_image_in_rect(ctx, is_highlighed?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
#else
    draw_image_in_rect(ctx, is_inverted, RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
#endif
    
    // Time
    GRect frame_time = GRect(0,
                             TEXT_Y_OFFSET,
                             bounds.size.w,
                             CELL_HEIGHT_2);
    
    if (str_time && str_time[0] != '\0') {
        GSize time_size = graphics_text_layout_get_content_size(str_time,
                                                                fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                                                                frame_time,
                                                                GTextOverflowModeTrailingEllipsis,
                                                                GTextAlignmentRight);
        frame_time.origin.x = bounds.size.w - CELL_MARGIN - time_size.w;
        frame_time.size.w = time_size.w;
        draw_text(ctx, str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time, GTextAlignmentRight);
    } else {
        frame_time.origin.x = bounds.size.w;
    }
    
    //  Draw station text, considering the scrolling index
    GRect frame_station = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                                TEXT_Y_OFFSET,
                                frame_time.origin.x - CELL_MARGIN_3 - FROM_TO_ICON_WIDTH,
                                CELL_HEIGHT_2);
    
    draw_text(ctx,
              is_selected?text_scroll_text(str_station, FONT_KEY_GOTHIC_18, frame_station):str_station,
              FONT_KEY_GOTHIC_18,
              frame_station,
              GTextAlignmentLeft);
    
    // Scroll texts
    if (is_selected) {
        text_scroll_begin(menu_layer_get_layer(menu_layer), str_station, strlen(str_station), FONT_KEY_GOTHIC_18, frame_station);
    }
}

// MARK: Menu Layer Callbacks

void menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex old_index = menu_layer_get_selected_index(context);
    if (old_index.section == 0 && old_index.row == 0) {
        menu_layer_set_selected_index(context, MenuIndex(UINT16_MAX, UINT16_MAX), MenuRowAlignBottom, true);
    } else {
        menu_layer_set_selected_next(context, true, MenuRowAlignCenter, true);
    }
}

void menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex old_index = menu_layer_get_selected_index(context);
    menu_layer_set_selected_next(context, false, MenuRowAlignCenter, true);
    MenuIndex new_index = menu_layer_get_selected_index(context);
    if (menu_index_compare(&old_index, &new_index) == 0) {
        menu_layer_set_selected_index(context, MenuIndex(0, 0), MenuRowAlignTop, true);
    }
}

#if !defined(PBL_PLATFORM_APLITE)

int16_t menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 1;
}

void menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

void menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context) {
    GRect frame = layer_get_frame(bg_layer);
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
}
#endif

// MARK: Scroll texts

#define TEXT_SCROLL_INTERVAL 300
#define TEXT_PAUSE_INTERVAL 600

static size_t s_text_scroll_index;
static size_t s_text_scroll_reset_index;
static AppTimer *s_text_scroll_timer;

static void text_scroll_timer_callback(Layer *layer) {
    if (s_text_scroll_index >= s_text_scroll_reset_index) {
        s_text_scroll_index = 0;
    } else {
        s_text_scroll_index += 1;
    }
    
    layer_mark_dirty(layer);
    
    s_text_scroll_timer = app_timer_register((s_text_scroll_index >= s_text_scroll_reset_index)?TEXT_PAUSE_INTERVAL:TEXT_SCROLL_INTERVAL, (AppTimerCallback)text_scroll_timer_callback, layer);
}

void text_scroll_begin(Layer *menu_layer, const char* text, size_t const text_length, const char * font_key, const GRect text_frame) {
    if (s_text_scroll_timer) {
        return;
    }
    
    s_text_scroll_index = s_text_scroll_reset_index = 0;
    GRect frame_test = GRect(0, 0, INT16_MAX, text_frame.size.h);
    GSize text_size = graphics_text_layout_get_content_size(text+s_text_scroll_reset_index, fonts_get_system_font(font_key), frame_test, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    while (s_text_scroll_reset_index < text_length &&
           (text_size.w > text_frame.size.w || text_size.w == 0)) {
        ++s_text_scroll_reset_index;
        text_size = graphics_text_layout_get_content_size(text+s_text_scroll_reset_index, fonts_get_system_font(font_key), frame_test, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    }
    if (s_text_scroll_reset_index > 0) {
        s_text_scroll_timer = app_timer_register(TEXT_SCROLL_INTERVAL, (AppTimerCallback)text_scroll_timer_callback, menu_layer);
    }
}

void text_scroll_end() {
    if (s_text_scroll_timer) {
        app_timer_cancel(s_text_scroll_timer);
        s_text_scroll_timer = NULL;
    }
}

size_t text_scroll_index() {
    return s_text_scroll_index;
}

char *text_scroll_text(char* text, const char * font_key, const GRect text_frame) {
    char *drawing_text = text + text_scroll_index();
    GRect frame_test = GRect(0, 0, INT16_MAX, text_frame.size.h);
    GSize text_size = graphics_text_layout_get_content_size(drawing_text, fonts_get_system_font(FONT_KEY_GOTHIC_18), frame_test, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    // In case of accent letters like é, à, we have to jump 2 digits
    if (text_size.w == 0) {
        drawing_text += 1;
        ++s_text_scroll_index;
    }
    return drawing_text;
}