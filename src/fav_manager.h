//
//  data_manager.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define FAV_MAX_COUNT 42// 6*42=252 < 256

typedef struct Favorite {
    char from[3];
    char to[3];
} Favorite;

void load_favorites();
void unload_favorites();
int16_t fav_count();
Favorite fav_at_index(int16_t index);
void fav_add(char *from, char *to);
void fav_delete_at_index(int16_t index);