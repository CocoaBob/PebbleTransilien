//
//  drawing.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment);

#ifdef PBL_SDK_3
void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **battery_layer);
#endif

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color);

void draw_image_in_rect(GContext* ctx, uint32_t resource_id, GRect rect);