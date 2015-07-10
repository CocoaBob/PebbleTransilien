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

// true is dark theme, false is light theme
bool get_theme_setting() {
    return persist_read_bool(SETTING_THEME);
}

void set_theme_setting(bool is_dark) {
    persist_write_bool(SETTING_THEME, is_dark);

    load_status();
}

bool get_locale_setting(char *locale) {
    if (persist_exists(SETTING_LOCALE)) {
        int result = persist_read_string(SETTING_LOCALE, locale, sizeof(locale));
        if (result != E_DOES_NOT_EXIST && result > 0) {
            return true;
        }
    }
    return false;
}

void set_locale_setting(const char* locale) {
    persist_write_string(SETTING_LOCALE, locale);
    
    load_status();
}