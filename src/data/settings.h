//
//  settings.h
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

void settings_toggle_locale();

bool settings_is_fav_on_launch();
void settings_toggle_is_fav_on_launch();

GColor curr_fg_color();
GColor curr_bg_color();