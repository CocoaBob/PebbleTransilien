//
//  ui_common.c
//  PebbleTransilien
//
//  Created by CocoaBob on 13/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

// MARK: Routines

GSize size_of_text(const char *text, const char *font_key, GRect frame) {
    return graphics_text_layout_get_content_size(text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

// MARK: Darw Basics

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment) {
    graphics_draw_text(ctx, text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

// MARK: Draw header and separators

void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color) {
    graphics_context_set_stroke_color(ctx, color);
    for (int16_t dx = 0; dx < layer_get_bounds(cell_layer).size.w; dx+=2) {
        graphics_draw_pixel(ctx, GPoint(dx, 0));
    }
}

// MARK: Draw images

void draw_image_in_rect(GContext* ctx,
                        uint32_t resource_id,
                        GRect rect) {
    GBitmap *bitmap = gbitmap_create_with_resource(resource_id);

    graphics_context_set_compositing_mode(ctx, GCompOpSet);

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
    if (is_inverted) {
        graphics_context_set_fill_color(ctx, curr_bg_color());
        graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    }
    graphics_context_set_text_color(ctx, curr_fg_color());

    draw_text(ctx, title, font_id?font_id:FONT_KEY_GOTHIC_18_BOLD, frame, GTextAlignmentCenter);
}

// MARK: Draw From To Layer, layer height should be 44

#if MINI_TIMETABLE_IS_ENABLED

#define NEXT_TRAINS_TIME_STR_LENGTH 5
#define NEXT_TRAINS_TIME_WIDTH 20
#define NEXT_TRAINS_PLATFORM_WIDTH 15

static void draw_from_to_next_train(GContext* ctx,
                                    bool is_inverted,
                                    GRect station_frame,
                                    DataModelMiniTimetable *next_trains,
                                    bool is_first) {
    GRect frame_next_train = GRect(station_frame.origin.x + station_frame.size.w,
                                   station_frame.origin.y + 4,
                                   NEXT_TRAINS_TIME_WIDTH,
                                   station_frame.size.h);
    char *str_next_train = calloc(NEXT_TRAINS_TIME_STR_LENGTH, sizeof(char));
    if (next_trains && next_trains[is_first?0:1].hour) {
        snprintf(str_next_train, NEXT_TRAINS_TIME_STR_LENGTH, "%ld'", relative_time((long)next_trains[is_first?0:1].hour));
    } else {
        snprintf(str_next_train, NEXT_TRAINS_TIME_STR_LENGTH, "--'");
    }
    draw_text(ctx, str_next_train, FONT_KEY_GOTHIC_14, frame_next_train, GTextAlignmentRight);
    free(str_next_train);
    
    GRect frame_platform = GRect(frame_next_train.origin.x + frame_next_train.size.w + 2, // Intenal margin = 2
                                 station_frame.origin.y + 6,
                                 NEXT_TRAINS_PLATFORM_WIDTH,
                                 NEXT_TRAINS_PLATFORM_WIDTH);
    graphics_context_set_stroke_color(ctx, is_inverted?GColorWhite:GColorBlack);
    graphics_draw_round_rect(ctx, frame_platform, 2);
    
    // Draw platform text
    if (next_trains) {
        frame_platform.origin.y -= 2;
        draw_text(ctx, next_trains[is_first?0:1].platform, FONT_KEY_GOTHIC_14, frame_platform, GTextAlignmentCenter);
    }
}

#endif

void draw_from_to(GContext* ctx, Layer *display_layer,
#if TEXT_SCROLL_IS_ENABLED
                  Layer *redraw_layer, bool is_selected,
#endif
                  bool is_inverted,
                  GColor text_color,
                  DataModelFromTo from_to
#if MINI_TIMETABLE_IS_ENABLED
                  ,
                  bool draw_mini_timetable
#endif
                  ) {
    graphics_context_set_text_color(ctx, text_color);
    GRect bounds = layer_get_bounds(display_layer);
    bool is_from_to = (from_to.to != STATION_NON);
    
    ////////////////////////////
    // Draw left icon
    ////////////////////////////
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT - FROM_TO_ICON_HEIGHT) / 2,
                             FROM_TO_ICON_WIDTH,
                             is_from_to?FROM_TO_ICON_HEIGHT:FROM_TO_ICON_WIDTH);
    if (is_from_to) {
        draw_image_in_rect(ctx, is_inverted?RESOURCE_ID_IMG_FROM_TO_DARK:RESOURCE_ID_IMG_FROM_TO_LIGHT, frame_icon);
    } else {
        draw_image_in_rect(ctx, is_inverted?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    }
    
    ////////////////////////////
    // Draw stations
    ////////////////////////////
    // Prepare strings and frames
    char *str_from = calloc(1, sizeof(char) * STATION_NAME_MAX_LENGTH);
    char *str_to = calloc(1, sizeof(char) * STATION_NAME_MAX_LENGTH);
    
    stations_get_name(from_to.from, str_from, STATION_NAME_MAX_LENGTH);
    GRect frame_from = GRect(CELL_MARGIN + FROM_TO_ICON_WIDTH + CELL_MARGIN,
                             TEXT_Y_OFFSET + 2, // +2 to let the two station names be closer
#if MINI_TIMETABLE_IS_ENABLED
                             bounds.size.w - FROM_TO_ICON_WIDTH - CELL_MARGIN_3 - (draw_mini_timetable?NEXT_TRAINS_TIME_WIDTH+2+NEXT_TRAINS_PLATFORM_WIDTH:0),// Next train intenal margin = 2
#else
                             bounds.size.w - FROM_TO_ICON_WIDTH - CELL_MARGIN_3,
#endif
                             CELL_HEIGHT_2);
    GRect frame_to = frame_from;
    frame_to.origin.y = CELL_HEIGHT_2 + TEXT_Y_OFFSET - 2; // -2 to let the two station names be closer
    if (is_from_to) {
        stations_get_name(from_to.to, str_to, STATION_NAME_MAX_LENGTH);
    } else {
        frame_from.size.h = bounds.size.h;
        GSize text_size = size_of_text(str_from, FONT_KEY_GOTHIC_18_BOLD, frame_from);
        frame_from.size.h = text_size.h;
    }
    
#if TEXT_SCROLL_IS_ENABLED
    // Scroll the station strings
    if (is_selected) {
        GRect frame_test = GRect(0, 0, INT16_MAX, frame_from.size.h);

        int16_t w_from = size_of_text(str_from, FONT_KEY_GOTHIC_18_BOLD, frame_test).w;
        int16_t w_to = size_of_text(str_to, FONT_KEY_GOTHIC_18_BOLD, frame_test).w;
        
        if (!text_scroll_is_on()) {
            GRect frame_scroll;
            if (w_from > w_to) {
                frame_scroll = frame_from;
            } else {
                frame_scroll = frame_to;
            }
            char **string_pointers = calloc(2, sizeof(char *));
            string_pointers[0] = str_from;
            string_pointers[1] = str_to;
            text_scroll_begin(redraw_layer, string_pointers, 2, FONT_KEY_GOTHIC_18_BOLD, frame_scroll);
            free(string_pointers);
        }
    }
#endif
    
    // Draw the station strings
    if (from_to.from != STATION_NON) {
        draw_text(ctx,
#if TEXT_SCROLL_IS_ENABLED
                  is_selected?text_scroll_text(str_from, 0, FONT_KEY_GOTHIC_18_BOLD, frame_from, false):str_from,
#else
                  str_from,
#endif
                  FONT_KEY_GOTHIC_18_BOLD,
                  frame_from,
                  GTextAlignmentLeft);
    }
    if (is_from_to) {
        draw_text(ctx,
#if TEXT_SCROLL_IS_ENABLED
                  is_selected?text_scroll_text(str_to, 1,FONT_KEY_GOTHIC_18_BOLD, frame_from, false):str_to,
#else
                  str_to,
#endif
                  FONT_KEY_GOTHIC_18_BOLD,
                  frame_to,
                  GTextAlignmentLeft);
    }
    
    // Free memory
    free(str_from);
    free(str_to);
                      
#if MINI_TIMETABLE_IS_ENABLED
    ////////////////////////////
    // Draw next train times
    ////////////////////////////
    if (draw_mini_timetable) {
        DataModelMiniTimetable *next_trains = fav_get_mini_timetables((Favorite)from_to);
        draw_from_to_next_train(ctx,
                                is_inverted,
                                frame_from,
                                next_trains,
                                true);
        draw_from_to_next_train(ctx,
                                is_inverted,
                                frame_to,
                                next_trains,
                                false);
    }
#endif
}

// MARK: Draw Station layer, layer hight should be 22

void draw_station(GContext *ctx, Layer *drawing_layer,
#if TEXT_SCROLL_IS_ENABLED
                  Layer *redraw_layer, bool is_selected,
#endif
                  GColor text_color,
                  bool is_inverted,
                  char * str_time,
                  char * str_station) {
    GRect bounds = layer_get_bounds(drawing_layer);

    graphics_context_set_text_color(ctx, text_color);
    
    // Icon
    GRect frame_icon = GRect(CELL_MARGIN,
                             (CELL_HEIGHT_2 - FROM_TO_ICON_WIDTH) / 2,
                             FROM_TO_ICON_WIDTH,
                             FROM_TO_ICON_WIDTH);
    draw_image_in_rect(ctx, is_inverted?RESOURCE_ID_IMG_FROM_DARK:RESOURCE_ID_IMG_FROM_LIGHT, frame_icon);
    
    // Time
    GRect frame_time = GRect(0,
                             TEXT_Y_OFFSET,
                             bounds.size.w,
                             CELL_HEIGHT_2);
    
    if (str_time && str_time[0] != '\0') {
        GSize time_size = size_of_text(str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time);
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
#if TEXT_SCROLL_IS_ENABLED
              is_selected?text_scroll_text(str_station, 0,FONT_KEY_GOTHIC_18, frame_station, true):str_station,
#else
              str_station,
#endif
              FONT_KEY_GOTHIC_18,
              frame_station,
              GTextAlignmentLeft);
    
#if TEXT_SCROLL_IS_ENABLED
    // Scroll texts
    if (is_selected) {
        char **string_pointers = calloc(1, sizeof(char *));
        string_pointers[0] = str_station;
        text_scroll_begin(redraw_layer, string_pointers, 1, FONT_KEY_GOTHIC_18, frame_station);
        free(string_pointers);
    }
#endif
}

// MARK: Menu Layer Callbacks

void common_menu_layer_button_up_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex old_index = menu_layer_get_selected_index(context);
    if (old_index.section == 0 && old_index.row == 0) {
        menu_layer_set_selected_index(context, MenuIndex(UINT16_MAX, UINT16_MAX), MenuRowAlignBottom, true);
    } else {
        menu_layer_set_selected_next(context, true, MenuRowAlignCenter, true);
    }
}

void common_menu_layer_button_down_handler(ClickRecognizerRef recognizer, void *context) {
    MenuIndex old_index = menu_layer_get_selected_index(context);
    menu_layer_set_selected_next(context, false, MenuRowAlignCenter, true);
    MenuIndex new_index = menu_layer_get_selected_index(context);
    if (menu_index_compare(&old_index, &new_index) == 0) {
        menu_layer_set_selected_index(context, MenuIndex(0, 0), MenuRowAlignTop, true);
    }
}

int16_t common_menu_layer_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    return 1;
}

void common_menu_layer_draw_separator_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    draw_separator(ctx, cell_layer, curr_fg_color());
}

void common_menu_layer_draw_background_callback(GContext* ctx, const Layer *bg_layer, bool highlight, void *callback_context) {
    GRect frame = layer_get_frame(bg_layer);
    graphics_context_set_fill_color(ctx, curr_bg_color());
    graphics_fill_rect(ctx, frame, 0, GCornerNone);
}

#if TEXT_SCROLL_IS_ENABLED

// MARK: Scroll texts

#define TEXT_SCROLL_INTERVAL 100
#define TEXT_PAUSE_INTERVAL 1000

static size_t s_text_scroll_index;              // Current scrolling positions
static size_t s_text_scroll_reset_index_max;    // Largest reset position
static size_t** s_text_scroll_reset_indexes;    // Reset positions, when scrolled to reset positions, return to the beginning
static size_t s_text_scroll_reset_indexes_count;
static AppTimer *s_text_scroll_timer;

static void text_scroll_timer_callback(Layer *layer) {
    if (s_text_scroll_index >= s_text_scroll_reset_index_max) {
        s_text_scroll_index = 0;
    } else {
        s_text_scroll_index += 1;
    }
    
    layer_mark_dirty(layer);
    
    // Pause at the beginning or end
    uint32_t timeout_ms = (s_text_scroll_index == 0 || s_text_scroll_index >= s_text_scroll_reset_index_max)?TEXT_PAUSE_INTERVAL:TEXT_SCROLL_INTERVAL;
    s_text_scroll_timer = app_timer_register(timeout_ms, (AppTimerCallback)text_scroll_timer_callback, layer);
}

void text_scroll_begin(Layer *redraw_layer, char** string_pointers, size_t text_count, const char * font_key, const GRect text_frame) {
    if (text_scroll_is_on()) {
        return;
    }
    
    // In case text_scroll_end() not called before calling text_scroll_begin()
    text_scroll_end();
    
    s_text_scroll_reset_indexes_count = text_count;
    
    s_text_scroll_index = s_text_scroll_reset_index_max = 0;
    s_text_scroll_reset_indexes = calloc(s_text_scroll_reset_indexes_count, sizeof(size_t*));
    for (size_t i = 0; i < s_text_scroll_reset_indexes_count; ++i) {
        s_text_scroll_reset_indexes[i] = (size_t *)calloc(1, sizeof(size_t));
    }
    GRect frame_test = GRect(0, 0, text_frame.size.w, INT16_MAX);
    for (size_t i = 0; i < s_text_scroll_reset_indexes_count; ++i) {
        GSize text_size = size_of_text((char *)string_pointers[i] + *((size_t*)s_text_scroll_reset_indexes[i]), font_key, frame_test);
        while (*((size_t*)s_text_scroll_reset_indexes[i]) < strlen((char *)string_pointers[i]) &&
               (text_size.h > text_frame.size.h || text_size.w == 0)) {
            ++*((size_t*)s_text_scroll_reset_indexes[i]);
            text_size = size_of_text((char *)string_pointers[i] + *((size_t*)s_text_scroll_reset_indexes[i]), font_key, frame_test);
            
        }
        s_text_scroll_reset_index_max = MAX(s_text_scroll_reset_index_max, *((size_t*)s_text_scroll_reset_indexes[i]));
    }
    for (size_t i = 0; i < s_text_scroll_reset_indexes_count; ++i) {
        if (*((size_t*)s_text_scroll_reset_indexes[i]) > 0) {
            s_text_scroll_timer = app_timer_register(TEXT_SCROLL_INTERVAL, (AppTimerCallback)text_scroll_timer_callback, redraw_layer);
            break;
        }
    }
}

void text_scroll_end() {
    if (s_text_scroll_reset_indexes != NULL) {
        for (size_t i = 0; i < s_text_scroll_reset_indexes_count; ++i) {
            NULL_FREE(s_text_scroll_reset_indexes[i]);
        }
        NULL_FREE(s_text_scroll_reset_indexes);
    }
    if (s_text_scroll_timer) {
        app_timer_cancel(s_text_scroll_timer);
        s_text_scroll_timer = NULL;
    }
}

bool text_scroll_is_on() {
    return s_text_scroll_timer != NULL;
}

char *text_scroll_text(char* text, size_t text_index, const char * font_key, const GRect text_frame, bool jump_accent) {
    if (!s_text_scroll_reset_indexes) {
        return text;
    }
    char *drawing_text = text + MIN(*((size_t*)s_text_scroll_reset_indexes[text_index]), s_text_scroll_index);
    GRect frame_test = GRect(0, 0, INT16_MAX, text_frame.size.h);
    GSize text_size = size_of_text(drawing_text, FONT_KEY_GOTHIC_18, frame_test);
    // In case of accent letters like é/è/à, we have to jump 2 digits
    if (text_size.w == 0) {
        drawing_text += 1;
        if (jump_accent) {
            ++s_text_scroll_index;
        }
    }
    return drawing_text;
}
#endif

// MARK: Push windows

bool ui_can_push_window() {
//    printf("1 Heap Total <%4dB> Used <%4dB> Free <%4dB>",heap_bytes_used()+heap_bytes_free(),heap_bytes_used(),heap_bytes_free());
    
#if !defined(PBL_PLATFORM_APLITE)
    size_t critical_memory = 3200;
#else
    size_t critical_memory = 2000;
#endif
        
    if (heap_bytes_free() < critical_memory) {
            // Show warning
        status_bar_low_memory_alert();
        
        // Vibrate
        vibes_enqueue_custom_pattern((VibePattern){
            .durations = (uint32_t[]) {50, 50, 50, 50, 50},
            .num_segments = 5
        });
        return false;
    }
    return true;
}

void ui_push_window(Window *window) {
    if (window != NULL) {
        window_stack_push(window, true);
    }
}
