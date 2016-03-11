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
    if (frame.size.w <= 0 || frame.size.h <= 0) {
        return GSizeZero;
    }
    return graphics_text_layout_get_content_size(text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

// MARK: Darw Basics

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment) {
    graphics_draw_text(ctx, text, fonts_get_system_font(font_key), frame, GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

// MARK: Draw header and separators

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color) {
    GRect bounds = layer_get_bounds(cell_layer);
    GRect frame = GRect(2,
                        TEXT_Y_OFFSET,
                        bounds.size.w - CELL_MARGIN_2,
                        bounds.size.h);
    
    graphics_context_set_text_color(ctx, color);
#ifdef PBL_ROUND
    draw_text(ctx, title, FONT_KEY_GOTHIC_14_BOLD, frame, GTextAlignmentCenter);
#else
    draw_text(ctx, title, FONT_KEY_GOTHIC_14_BOLD, frame, GTextAlignmentLeft);
#endif
}

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

// MARK: Text Scroll callback
#if TEXT_SCROLL_IS_ENABLED
static void text_scroll_update(TextScrollConfig *config) {
    if (config->user_context != NULL) {
        layer_mark_dirty(config->user_context);
    }
}
#endif

// MARK: Draw From To Layer, layer height should be 44

#if MINI_TIMETABLE_IS_ENABLED

#define NEXT_TRAINS_TIME_STR_LENGTH 5
#define NEXT_TRAINS_TIME_WIDTH 20
#define NEXT_TRAINS_PLATFORM_WIDTH 15

static void draw_from_to_next_train(GContext* ctx,
                                    GColor text_color,
                                    bool is_inverted,
                                    GRect station_frame,
                                    DataModelMiniTimetable *next_trains,
                                    bool is_first) {
    // Draw text
    GRect frame_next_train = GRect(station_frame.origin.x + station_frame.size.w,
                                   station_frame.origin.y + 4,
                                   NEXT_TRAINS_TIME_WIDTH,
                                   station_frame.size.h);
    char *str_next_train = calloc(NEXT_TRAINS_TIME_STR_LENGTH, sizeof(char));
    if (next_trains && next_trains[is_first?0:1].hour) {
        long time = relative_time((long)next_trains[is_first?0:1].hour);
        if (time > 99 || time < -99) {
            strncpy(str_next_train, (time > 99) ? "FF'" : "-FF'", 4);
        } else {
            snprintf(str_next_train, NEXT_TRAINS_TIME_STR_LENGTH, "%ld'", time);
        }
    } else {
        strncpy(str_next_train, "--'", 3);
    }
    draw_text(ctx, str_next_train, FONT_KEY_GOTHIC_14, frame_next_train, GTextAlignmentRight);
    free(str_next_train);
    
    // Draw platform frame
    GRect frame_platform = GRect(frame_next_train.origin.x + frame_next_train.size.w + 2, // Intenal margin = 2
                                 station_frame.origin.y + 6,
                                 NEXT_TRAINS_PLATFORM_WIDTH,
                                 NEXT_TRAINS_PLATFORM_WIDTH);
    graphics_context_set_stroke_color(ctx, text_color);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_round_rect(ctx, frame_platform, 2);
    
    // Draw platform text
    frame_platform.origin.y -= 2; // Adjust text baseline
    draw_text(ctx, next_trains ? next_trains[is_first?0:1].platform : "?", FONT_KEY_GOTHIC_14, frame_platform, GTextAlignmentCenter);
    
    // Draw an indicator for "mentioned" status
    if (next_trains && next_trains[is_first?0:1].mentioned) {
        GRect frame_mentioned = GRect(frame_platform.origin.x + frame_platform.size.w,
                                      frame_platform.origin.y,
                                      CELL_MARGIN,
                                      NEXT_TRAINS_PLATFORM_WIDTH);
        draw_text(ctx, "!", FONT_KEY_GOTHIC_14_BOLD, frame_mentioned, GTextAlignmentLeft);
    }
}

#endif

void draw_from_to(GContext* ctx, Layer *display_layer,
#if TEXT_SCROLL_IS_ENABLED
                  TextScrollData **ptr_text_scroll_data, Layer *redraw_layer, bool is_selected,
#endif
                  bool is_inverted,
                  GColor bg_color, GColor fg_color,
                  DataModelFromTo from_to
#if MINI_TIMETABLE_IS_ENABLED
                  ,
                  bool draw_mini_timetable
#endif
#if EXTRA_INFO_IS_ENABLED
                  ,
                  bool draw_extra_info_indicator
#endif
                  )
{
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_context_set_text_color(ctx, fg_color);
    GRect bounds = layer_get_bounds(display_layer);
    bool is_from_to = (from_to.to != STATION_NON);
    
    ////////////////////////////
    // Draw left icon
    ////////////////////////////
    uint16_t icon_radius = FROM_TO_ICON_SIZE / 2;
    GPoint icon_center_from = GPoint(CELL_MARGIN + icon_radius, 0               + CELL_HEIGHT_4 + 2); // +2 to let the two station names be closer
    GPoint icon_center_to   = GPoint(CELL_MARGIN + icon_radius, CELL_HEIGHT_2   + CELL_HEIGHT_4 - 2); // -2 to let the two station names be closer
    
#ifdef PBL_ROUND
    GRect window_bounds = layer_get_bounds(window_get_root_layer(layer_get_window(display_layer)));
    
    // Calculate icon's x positions for PBL_ROUND
    GPoint icon_angle_point_from = GPoint(0, layer_convert_point_to_screen(display_layer, icon_center_from).y);
    icon_angle_point_from.x = get_round_border_x_radius_82(icon_angle_point_from.y - CELL_MARGIN_2) + CELL_MARGIN_2;
    icon_center_from.x = icon_angle_point_from.x;
    
    GPoint icon_angle_point_to = GPoint(0, layer_convert_point_to_screen(display_layer, icon_center_to).y);
    icon_angle_point_to.x = get_round_border_x_radius_82(icon_angle_point_to.y - CELL_MARGIN_2) + CELL_MARGIN_2;
    icon_center_to.x = icon_angle_point_to.x;
#endif
    
    // Draw icons
    if (is_from_to) {
#ifdef PBL_ROUND
        if (icon_angle_point_from.y >= 0 && icon_angle_point_from.y <= ROUND_SCREEN_SIZE &&
            icon_angle_point_to.y >= 0 && icon_angle_point_to.y <= ROUND_SCREEN_SIZE) {
            int32_t quad_circle = TRIG_MAX_ANGLE / 4;
            
            int32_t angle_from = atan2_lookup(ABS(ROUND_SCREEN_RADIUS - icon_angle_point_from.y), ABS(ROUND_SCREEN_RADIUS - icon_angle_point_from.x));
            angle_from = (icon_angle_point_from.y > ROUND_SCREEN_RADIUS) ? (-quad_circle-angle_from) : (angle_from-quad_circle);
            
            int32_t angle_to = atan2_lookup(ABS(ROUND_SCREEN_RADIUS - icon_angle_point_to.y), ABS(ROUND_SCREEN_RADIUS - icon_angle_point_to.x));
            angle_to = (icon_angle_point_to.y > ROUND_SCREEN_RADIUS) ? (-quad_circle-angle_to) : (angle_to-quad_circle);
            GRect draw_rect = window_bounds;
            GPoint layer_origin_screen = layer_convert_point_to_screen(display_layer, GPointZero);
            draw_rect.origin.x -= layer_origin_screen.x;
            draw_rect.origin.y -= layer_origin_screen.y;
            draw_rect = grect_inset(draw_rect, GEdgeInsets1(CELL_MARGIN_2-1));
            
//            graphics_draw_arc(ctx, draw_rect, GOvalScaleModeFillCircle, angle_to, angle_from);
            graphics_context_set_fill_color(ctx, fg_color);
            graphics_fill_radial(ctx, draw_rect, GOvalScaleModeFillCircle, 3, angle_to, angle_from);
        }
#else
        graphics_context_set_stroke_width(ctx, 2);
        graphics_draw_line(ctx, icon_center_from, icon_center_to);
#endif
    }
    
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_fill_circle(ctx, icon_center_from, icon_radius);
    if (is_from_to) {
        graphics_fill_circle(ctx, icon_center_to, icon_radius);
    }
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_circle(ctx, icon_center_from, icon_radius);
    if (is_from_to) {
        graphics_draw_circle(ctx, icon_center_to, icon_radius);
    }
    
    ////////////////////////////
    // Draw stations
    ////////////////////////////
    // Prepare strings and frames
    char *str_from = calloc(1, sizeof(char) * STATION_NAME_MAX_LENGTH);
    char *str_to = calloc(1, sizeof(char) * STATION_NAME_MAX_LENGTH);
    
    stations_get_name(from_to.from, str_from, STATION_NAME_MAX_LENGTH);
    GRect frame_from = GRect(CELL_MARGIN + FROM_TO_ICON_SIZE + CELL_MARGIN,
                             TEXT_Y_OFFSET + 2, // +2 to let the two station names be closer
                             bounds.size.w - FROM_TO_ICON_SIZE - CELL_MARGIN_3,
                             CELL_HEIGHT_2);
    
    // Inset for mini time table or extra info indicator
#if MINI_TIMETABLE_IS_ENABLED
    frame_from.size.w -= (draw_mini_timetable?NEXT_TRAINS_TIME_WIDTH+2+NEXT_TRAINS_PLATFORM_WIDTH:0); // Next train intenal margin = 2
#endif
#if EXTRA_INFO_IS_ENABLED
    frame_from.size.w -= (draw_extra_info_indicator?CELL_SUB_ICON_SIZE:0);
#endif
    
    GRect frame_to = frame_from;
    frame_to.origin.y = CELL_HEIGHT_2 + TEXT_Y_OFFSET - 2; // -2 to let the two station names be closer
    
#ifdef PBL_ROUND
    // Adjust frames for PBL_ROUND
    int16_t line_1_inset = icon_center_from.x - CELL_MARGIN_2;
    int16_t line_2_inset = icon_center_to.x - CELL_MARGIN_2;
    
    frame_from.origin.x += line_1_inset + CELL_MARGIN;
    frame_from.size.w -= line_1_inset + line_1_inset + CELL_MARGIN_2;
    
    frame_to.origin.x += line_2_inset + CELL_MARGIN;
#if EXTRA_INFO_IS_ENABLED
    frame_to.size.w -= line_2_inset + (draw_extra_info_indicator?line_1_inset:line_2_inset) + CELL_MARGIN_2;
#else
    frame_to.size.w -= line_2_inset + line_2_inset + CELL_MARGIN_2;
#endif
#endif
    
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
        const char **texts = calloc(2, sizeof(char *));
        texts[0] = str_from;
        texts[1] = str_to;
        const GSize **sizes = calloc(2, sizeof(GSize *));
        sizes[0] = &(frame_from.size);
        sizes[1] = &(frame_to.size);
        const char **font_keys = calloc(2, sizeof(char *));
        font_keys[0] = FONT_KEY_GOTHIC_18_BOLD;
        font_keys[1] = FONT_KEY_GOTHIC_18_BOLD;
        if (!text_scroll_is_unchanged(texts, sizes, font_keys, 2, *ptr_text_scroll_data)) {
            text_scroll_create((TextScrollConfig) {
                .frame_interval = TEXT_FRAME_INTERVAL,
                .pause_interval = TEXT_PAUSE_INTERVAL,
                .update_callback = (TextScrollUpdateCb)text_scroll_update,
                .user_context = redraw_layer
            },
                               texts,
                               sizes,
                               font_keys,
                               2,
                               ptr_text_scroll_data);
        }
        free(texts);
        free(sizes);
        free(font_keys);
    }
#endif
    
    // Draw the station strings
    if (from_to.from != STATION_NON) {
        draw_text(ctx,
#if TEXT_SCROLL_IS_ENABLED
                  is_selected?text_scroll_text(str_from, 0, false, *ptr_text_scroll_data):str_from,
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
                  is_selected?text_scroll_text(str_to, 1, false, *ptr_text_scroll_data):str_to,
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
                                fg_color,
                                is_inverted,
                                frame_from,
                                next_trains,
                                true);
        draw_from_to_next_train(ctx,
                                fg_color,
                                is_inverted,
                                frame_to,
                                next_trains,
                                false);
    }
#endif
    
#if EXTRA_INFO_IS_ENABLED
    // Draw warning icon
    if (draw_extra_info_indicator) {
        GRect frame_mention = GRect(frame_from.origin.x + frame_from.size.w,
                                    CELL_HEIGHT_2 - CELL_SUB_ICON_SIZE / 2,
                                    CELL_SUB_ICON_SIZE,
                                    CELL_SUB_ICON_SIZE);
#ifdef PBL_ROUND
        frame_mention.origin.x += CELL_MARGIN;
#endif
        draw_image_in_rect(ctx, is_inverted?RESOURCE_ID_IMG_MENTION_DARK:RESOURCE_ID_IMG_MENTION_LIGHT, frame_mention);
    }
#endif
}

// MARK: Draw Station layer, layer hight should be 22

void draw_station(GContext *ctx, Layer *drawing_layer,
#if TEXT_SCROLL_IS_ENABLED
                  TextScrollData **ptr_text_scroll_data, Layer *redraw_layer, bool is_selected,
#endif
                  GColor text_color,
                  bool is_inverted,
                  char * str_time,
                  char * str_station) {
    GRect bounds = layer_get_bounds(drawing_layer);

    graphics_context_set_text_color(ctx, text_color);
    graphics_context_set_stroke_color(ctx, text_color);
    graphics_context_set_stroke_width(ctx, 1);
    
    // Icon
    uint16_t icon_radius = FROM_TO_ICON_SIZE / 2;
    GPoint icon_center = GPoint(CELL_MARGIN + icon_radius, CELL_HEIGHT_4);
    
#ifdef PBL_ROUND
    // Calculate icon's x positions for PBL_ROUND
    GPoint icon_angle_point = GPoint(0, layer_convert_point_to_screen(drawing_layer, icon_center).y);
    icon_angle_point.x = get_round_border_x_radius_82(icon_angle_point.y - CELL_MARGIN_2) + CELL_MARGIN_2;
    icon_center.x = icon_angle_point.x;
    
    int16_t row_inset = icon_center.x - CELL_MARGIN_2;
#endif
    
    // Draw icons
    graphics_draw_circle(ctx, icon_center, icon_radius);
    
    // Time
    GRect frame_time = GRect(0,
                             TEXT_Y_OFFSET,
                             bounds.size.w,
                             CELL_HEIGHT_2);
    
    if (str_time && str_time[0] != '\0') {
        GSize time_size = size_of_text(str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time);
        frame_time.size.w = time_size.w;
#ifdef PBL_ROUND
        frame_time.origin.x = bounds.size.w - row_inset - CELL_MARGIN - time_size.w;
        if (frame_time.origin.x > icon_center.x + icon_radius + CELL_MARGIN_2) {
            draw_text(ctx, str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time, GTextAlignmentRight);
        }
#else
        frame_time.origin.x = bounds.size.w - CELL_MARGIN - time_size.w;
        draw_text(ctx, str_time, FONT_KEY_GOTHIC_18_BOLD, frame_time, GTextAlignmentRight);
#endif
    } else {
        frame_time.origin.x = bounds.size.w;
    }
    
    //  Draw station text, considering the scrolling index
    GRect frame_station = GRect(CELL_MARGIN + FROM_TO_ICON_SIZE + CELL_MARGIN,
                                TEXT_Y_OFFSET,
                                frame_time.origin.x - CELL_MARGIN_3 - FROM_TO_ICON_SIZE,
                                CELL_HEIGHT_2);
    
#ifdef PBL_ROUND
    // Adjust frames for PBL_ROUND
    frame_station.origin.x += row_inset + CELL_MARGIN;
    frame_station.size.w -= row_inset + CELL_MARGIN;
#endif
    
#ifdef PBL_ROUND
    if (frame_station.size.w > 0) {
#endif
        draw_text(ctx,
#if TEXT_SCROLL_IS_ENABLED
                  is_selected?text_scroll_text(str_station, 0, true, *ptr_text_scroll_data):str_station,
#else
                  str_station,
#endif
                  FONT_KEY_GOTHIC_18,
                  frame_station,
                  GTextAlignmentLeft);
#ifdef PBL_ROUND
    }
#endif
    
#if TEXT_SCROLL_IS_ENABLED
    // Scroll texts
    if (is_selected) {
        const char **texts = calloc(1, sizeof(char *));
        texts[0] = str_station;
        const GSize **sizes = calloc(1, sizeof(GSize *));
        sizes[0] = &(frame_station.size);
        const char **font_keys = calloc(1, sizeof(char *));
        font_keys[0] = FONT_KEY_GOTHIC_18;
        if (!text_scroll_is_unchanged(texts, sizes, font_keys, 1, *ptr_text_scroll_data)) {
            text_scroll_create((TextScrollConfig) {
                .frame_interval = TEXT_FRAME_INTERVAL,
                .pause_interval = TEXT_PAUSE_INTERVAL,
                .update_callback = (TextScrollUpdateCb)text_scroll_update,
                .user_context = redraw_layer
            },
                               texts,
                               sizes,
                               font_keys,
                               1,
                               ptr_text_scroll_data);
        }
        free(texts);
        free(sizes);
        free(font_keys);
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
#ifndef PBL_ROUND // Space not enough for warning messages
        status_bar_low_memory_alert();
#endif
        
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
    if (window != NULL && ui_can_push_window()) {
        window_stack_push(window, true);
    }
}
