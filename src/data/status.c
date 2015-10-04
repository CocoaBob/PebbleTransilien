//
//  status.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static bool s_is_dark_theme;
static char* s_curr_locale;
static bool s_is_fav_on_launch;

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
    
    // Get is fave on launch
    s_is_fav_on_launch = storage_get_is_fav_on_launch();
}

void status_deinit() {
    NULL_FREE(s_curr_locale);
}


bool status_is_dark_theme() {
    return s_is_dark_theme;
}

char *status_curr_locale() {
    return s_curr_locale;
}

bool status_is_fav_on_launch() {
    return s_is_fav_on_launch;
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