//
//  storage.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

bool get_theme_setting();
void set_theme_setting(bool is_dark);

bool get_locale_setting(char *locale);
void set_locale_setting(const char* lang);