//
//  app_settings.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"


typedef struct {
    bool is_dark_theme;
    char* curr_locale;
    bool mini_timetable_is_disabled;
} Settings;

static Settings *s_settings;

void settings_init() {
    s_settings = calloc(1, sizeof(Settings));
    
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
    s_settings->is_dark_theme = persist_read_bool(SETTING_KEY_THEME);
    
    // Get mini timetable status
    s_settings->mini_timetable_is_disabled = persist_read_bool(SETTING_KEY_DISABLE_MINI_TIMETABLE);
    
    // Get current locale
    char *_curr_locale = calloc(6, sizeof(char));
    persist_read_string(SETTING_KEY_LOCALE, _curr_locale, 6);
    s_settings->curr_locale = setlocale(LC_ALL, _curr_locale);
    NULL_FREE(_curr_locale);
}

void settings_deinit() {
    NULL_FREE(s_settings);
}

bool settings_is_dark_theme() {
    return s_settings->is_dark_theme;
}

void settings_set_theme(bool is_dark) {
    persist_write_bool(SETTING_KEY_THEME, is_dark);
    s_settings->is_dark_theme = is_dark;
}

#if MINI_TIMETABLE_IS_ENABLED
bool settings_mini_timetable_is_enabled() {
    return !s_settings->mini_timetable_is_disabled;
}

void settings_set_mini_timetable_enable(bool is_enabled) {
    s_settings->mini_timetable_is_disabled = !is_enabled;
    persist_write_bool(SETTING_KEY_DISABLE_MINI_TIMETABLE, s_settings->mini_timetable_is_disabled);
}
#endif

void settings_toggle_locale() {
    s_settings->curr_locale = setlocale(LC_ALL, (strncmp(s_settings->curr_locale, "en", 2) == 0)?"fr_FR":"en_US");
    persist_write_string(SETTING_KEY_LOCALE, s_settings->curr_locale);
    locale_deinit();
    locale_init();
}

GColor curr_fg_color() {
#ifdef PBL_COLOR
    return s_settings->is_dark_theme?GColorWhite:GColorBlack;
#else
    return GColorBlack;
#endif
}

GColor curr_bg_color() {
#ifdef PBL_COLOR
    return s_settings->is_dark_theme?GColorBlack:GColorWhite;
#else
    return GColorWhite;
#endif
}