//
//  storage.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

bool storage_get_theme();
void storage_set_theme(bool is_dark);

bool storage_get_locale(char *locale);
void storage_set_locale(const char* lang);

bool storage_get_favorites(void *favorites, const size_t buffer_size);
bool storage_set_favorites(const Favorite *favorites, int16_t fav_count);
int16_t storage_get_favorites_count();
bool storage_set_favorites_count(int16_t fav_count);
