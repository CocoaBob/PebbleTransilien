//
//  app_settings.c
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

void settings_init() {
    // Compare the version of station data
    // If it's not the same, reset favorites
    int32_t last_settings_version = persist_read_int(SETTING_KEY_STATION_DATA_VERSION);
    if (last_settings_version != 0 &&
        last_settings_version != STATION_DATA_VERSION) {
        persist_delete(SETTING_KEY_FAVORITES);
        persist_delete(SETTING_KEY_FAVORITES_COUNT);
    }
    persist_write_int(SETTING_KEY_STATION_DATA_VERSION, STATION_DATA_VERSION);
    
    // Get current theme
    s_is_dark_theme = persist_read_bool(SETTING_KEY_THEME);
    
    // Get current locale
    char *curr_locale = calloc(6, sizeof(char));
    persist_read_string(SETTING_KEY_LOCALE, curr_locale, 6);
    s_curr_locale = setlocale(LC_ALL, curr_locale);
    NULL_FREE(curr_locale);
    
    // Get is fave on launch
    s_is_fav_on_launch = persist_read_bool(SETTING_KEY_IS_FAV_ON_LAUNCH);
}

void settings_deinit() {
}

bool settings_is_dark_theme() {
    return s_is_dark_theme;
}

void settings_set_theme(bool is_dark) {
    persist_write_bool(SETTING_KEY_THEME, is_dark);
    s_is_dark_theme = is_dark;
}

void settings_toggle_locale() {
    s_curr_locale = setlocale(LC_ALL, (strncmp(s_curr_locale, "en", 2) == 0)?"fr_FR":"en_US");
    persist_write_string(SETTING_KEY_LOCALE, s_curr_locale);
    locale_deinit();
    locale_init();
}

bool settings_is_fav_on_launch() {
    return s_is_fav_on_launch;
}

void settings_toggle_is_fav_on_launch() {
    s_is_fav_on_launch = !s_is_fav_on_launch;
    persist_write_bool(SETTING_KEY_IS_FAV_ON_LAUNCH, s_is_fav_on_launch);
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