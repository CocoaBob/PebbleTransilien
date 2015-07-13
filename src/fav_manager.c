//
//  data_manager.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static Favorite *s_favorites;

void load_favorites() {
    if (s_favorites != NULL) {
        return;
    }
    s_favorites = malloc(PERSIST_DATA_MAX_LENGTH);
    if (!storage_get_favorites(s_favorites)) {
        unload_favorites();
    }
}

void unload_favorites() {
    if (s_favorites != NULL) {
        free(s_favorites);
        s_favorites = NULL;
    }
}

int16_t fav_get_count() {
    return storage_get_favorites_count();
}

Favorite fav_at_index(int16_t index) {
    return s_favorites[index];
}

bool fav_add(char *from, char *to) {
    // If s_favorites hasn't been loaded
    if (s_favorites == NULL) {
        load_favorites();
    }
    
    int16_t fav_count = fav_get_count();
    // If we already have max count of favorites
    if (fav_count >= FAV_MAX_COUNT) {
        return false;
    } else {
        fav_count += 1;
    }
    
    if (s_favorites == NULL) {
        // The 1st time
        s_favorites = malloc(sizeof(Favorite) * 1);
    } else {
        // Resize the existing array
        Favorite* new_favorites = realloc(s_favorites, sizeof(Favorite) * fav_count);
        if (new_favorites != NULL) {
            s_favorites = new_favorites;
        }
    }
    
    // If failed to allocate memory
    if (s_favorites == NULL) {
        return false;
    }
    
    // Copy values
    Favorite *new_fav = &s_favorites[fav_count - 1];
    strncpy(new_fav->from, from, 3);
    strncpy(new_fav->to, to, 3);
    
    // Save data
    storage_set_favorites_count(fav_count);
    storage_set_favorites(s_favorites, fav_count);
    
    return true;
}

bool fav_delete_at_index(int16_t index) {
    return false;
}