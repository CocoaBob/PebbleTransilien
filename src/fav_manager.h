//
//  data_manager.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define FAV_MAX_COUNT 32// 8*32=256, 256 is persist data max lenght

typedef struct Favorite {
    char from[4];
    char to[4];
} Favorite;

void load_favorites();
void unload_favorites();
int16_t fav_get_count();
Favorite fav_at_index(int16_t index);
bool fav_add(char *from, char *to);
bool fav_delete_at_index(int16_t index);