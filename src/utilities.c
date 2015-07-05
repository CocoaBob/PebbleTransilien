#include <pebble.h>
#include "utilities.h"

void draw_text(GContext *ctx, const char * text, GColor color, const char * font_key, GRect frame, GTextAlignment alignment) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx,
                     text,
                     fonts_get_system_font(font_key),
                     frame,
                     GTextOverflowModeTrailingEllipsis,
                     alignment,
                     NULL);
}