//
//  storage.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

#define SETTING_THEME 100
#define SETTING_LOCALE 101
#define SETTING_FAVORITES 102
#define SETTING_FAVORITES_COUNT 103

// true is dark theme, false is light theme
bool storage_get_theme() {
    return persist_read_bool(SETTING_THEME);
}

void storage_set_theme(bool is_dark) {
    persist_write_bool(SETTING_THEME, is_dark);

    status_init();
}

bool storage_get_locale(char *locale) {
    if (persist_exists(SETTING_LOCALE)) {
        int result = persist_read_string(SETTING_LOCALE, locale, sizeof(locale));
        if (result != E_DOES_NOT_EXIST && result > 0) {
            return true;
        }
    }
    return false;
}

void storage_set_locale(const char* locale) {
    persist_write_string(SETTING_LOCALE, locale);
    
    status_init();
}

bool storage_get_favorites(void *favorites, const size_t buffer_size) {
    if (persist_exists(SETTING_FAVORITES) &&
        persist_read_data(SETTING_FAVORITES, favorites, buffer_size) != E_DOES_NOT_EXIST) {
        return true;
    }
    return false;
}

bool storage_set_favorites(const Favorite *favorites, size_t fav_count) {
    int result = persist_write_data(SETTING_FAVORITES, favorites, size_of_Favorite() * fav_count);
    return (result == (int)(size_of_Favorite() * fav_count));
}

size_t storage_get_favorites_count() {
    return persist_read_int(SETTING_FAVORITES_COUNT);
}

bool storage_set_favorites_count(size_t fav_count) {
    status_t result = persist_write_int(SETTING_FAVORITES_COUNT, fav_count);
    return result == S_SUCCESS;
}