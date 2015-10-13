//
//  status.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

void status_init();
void status_deinit();

bool status_is_dark_theme();
void status_set_theme(bool is_dark);

void status_toggle_locale();

bool status_is_fav_on_launch();
void status_toggle_is_fav_on_launch();

GColor curr_fg_color();
GColor curr_bg_color();