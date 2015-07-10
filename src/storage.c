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
#define SETTING_FAVORITES 101

// true is dark theme, false is light theme
bool storage_get_theme() {
    return persist_read_bool(SETTING_THEME);
}

void storage_set_theme(bool is_dark) {
    persist_write_bool(SETTING_THEME, is_dark);

    load_status();
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
    
    load_status();
}

bool storage_get_favorites(void *favorites) {
    if (persist_exists(SETTING_FAVORITES)) {
        int result = persist_read_data(SETTING_FAVORITES, favorites, PERSIST_DATA_MAX_LENGTH);
        if (result != E_DOES_NOT_EXIST && result > 0) {
            return true;
        }
    }
    return false;
}

void storage_set_favorites(const void *favorites) {
    persist_write_data(SETTING_FAVORITES, favorites, sizeof(favorites));
}