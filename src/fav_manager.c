//
//  data_manager.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static Favorite *favorites;

void load_favorites() {
    unload_favorites();
    favorites = malloc(PERSIST_DATA_MAX_LENGTH);
    if (!storage_get_favorites(favorites)) {
        favorites = NULL;
    }
}

void unload_favorites() {
    if (favorites != NULL) {
        free(favorites);
        favorites = NULL;
    }
}

int16_t fav_count() {
    return sizeof(favorites) / sizeof(Favorite);
}

Favorite fav_at_index(int16_t index) {
    return favorites[index];
}

void fav_add(char *from, char *to) {
    Favorite* new_fav = (Favorite *)realloc(favorites, sizeof(favorites) + sizeof(Favorite));
    strcpy(new_fav->from, from);
    strcpy(new_fav->to, to);
}

void fav_delete_at_index(int16_t index) {
    
}