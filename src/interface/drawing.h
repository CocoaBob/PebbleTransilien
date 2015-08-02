//
//  drawing.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

void draw_text(GContext *ctx, const char * text, const char * font_key, GRect frame, GTextAlignment alignment);

void draw_menu_header(GContext *ctx, const Layer *cell_layer, const char * title, GColor color);
void draw_separator(GContext *ctx, const Layer *cell_layer, GColor color);

void draw_image_in_rect(GContext* ctx, uint32_t resource_id, GRect rect);

void draw_cell_title(GContext* ctx, const Layer *cell_layer, const char *title);

void draw_from_to_layer(GContext* ctx,
                        const Layer *layer,
                        DataModelFromTo from_to,
#ifdef PBL_COLOR
                        bool is_highlighed,
#endif
                        GColor text_color);