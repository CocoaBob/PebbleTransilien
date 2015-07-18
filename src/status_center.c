//
//  status_center.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static bool s_is_dark_theme;
static char* s_curr_locale;

void status_init() {
    status_deinit();
    
    // Get current theme
    s_is_dark_theme = storage_get_theme();
    
    // Get current locale
    s_curr_locale = malloc(sizeof(char) * 5);
    if (!storage_get_locale(s_curr_locale)) {
        s_curr_locale = NULL;
    }
    setlocale(LC_ALL, s_curr_locale);
}

void status_deinit() {
    if (s_curr_locale != NULL) {
        free(s_curr_locale);
        s_curr_locale = NULL;
    }
}


bool status_is_dark_theme() {
    return s_is_dark_theme;
}

char *status_curr_locale() {
    return s_curr_locale;
}

GColor curr_fg_color() {
#ifdef PBL_COLOR
    return s_is_dark_theme?GColorWhite:GColorBlack;
#else
    return GColorBlack;
#endif
}

GColor curr_bg_color() {
#ifdef PBL_COLOR
    return s_is_dark_theme?GColorBlack:GColorWhite;
#else
    return GColorWhite;
#endif
}