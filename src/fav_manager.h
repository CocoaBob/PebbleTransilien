//
//  data_manager.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define FAV_MAX_COUNT 32// 8*32=256, 256 is persist data max lenght
#define FAV_COMPONENT_LENGTH 4

typedef struct Favorite {
    char from[FAV_COMPONENT_LENGTH];
    char to[FAV_COMPONENT_LENGTH];
} Favorite;

size_t size_of_Favorite();
void load_favorites();
void unload_favorites();
int16_t fav_get_count();
Favorite fav_at_index(int16_t index);
bool fav_add(char *from, char *to);
bool fav_delete_at_index(int16_t index);