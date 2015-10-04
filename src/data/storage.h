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

bool storage_get_is_fav_on_launch();
void storage_set_is_fav_on_launch(bool is_fav_on_launch);

bool storage_get_favorites(void *favorites, const size_t buffer_size);
bool storage_set_favorites(const Favorite *favorites, size_t fav_count);
bool storage_delete_all_favorites();
size_t storage_get_favorites_count();
bool storage_set_favorites_count(size_t fav_count);
