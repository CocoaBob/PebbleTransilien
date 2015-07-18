//
//  data_manager.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define FAV_MAX_COUNT 64// 4*64=256, 256 is persist data max lenght

typedef struct Favorite {
    size_t from;
    size_t to;
} Favorite;

size_t size_of_Favorite();
void load_favorites();
void unload_favorites();
size_t fav_get_count();
Favorite fav_at_index(size_t index);
bool fav_add(size_t from, size_t to);
bool fav_delete_at_index(size_t index);