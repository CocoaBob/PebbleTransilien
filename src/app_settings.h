//
//  app_settings.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

void settings_init();
void settings_deinit();

bool settings_is_dark_theme();
void settings_set_theme(bool is_dark);

#if MINI_TIMETABLE_IS_ENABLED
bool settings_mini_timetable_is_enabled();
void settings_set_mini_timetable_enable(bool is_enabled);
#endif

void settings_toggle_locale();

GColor curr_fg_color();
GColor curr_bg_color();