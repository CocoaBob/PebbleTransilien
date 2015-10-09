//
//  drawing.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment);

#ifdef PBL_COLOR
void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);
#endif

#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_CHALK)
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color);
#endif

void draw_image_in_rect(GContext* ctx, uint32_t resource_id, GRect rect);

void draw_centered_title(GContext* ctx, const Layer *cell_layer, const char *title, const char *font_id, GColor color);

void draw_from_to(GContext* ctx,
                        const Layer *layer,
                        DataModelFromTo from_to,
#ifdef PBL_COLOR
                        bool is_highlighed,
#endif
                        GColor text_color);

void draw_station(GContext *ctx, Layer *cell_layer,
                  GColor text_color,
#ifdef PBL_COLOR
                  bool is_highlighed,
#endif
                  const char * str_time,
                  const char * str_station);