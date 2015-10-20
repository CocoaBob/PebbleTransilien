//
//  data_favorites.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static Favorite *s_favorites;

static void storage_set_favorites(const Favorite *favorites, size_t fav_count) {
    if (fav_count == 0) {
        persist_delete(SETTING_KEY_FAVORITES);
    } else {
        persist_write_data(SETTING_KEY_FAVORITES, favorites, sizeof(Favorite) * fav_count);
    }
}

void favorite_copy(Favorite *dest, Favorite *src) {
    dest->from = src->from;
    dest->to = src->to;
}

void favorites_init() {
    if (s_favorites != NULL) {
        return;
    }
    size_t buffer_size = sizeof(Favorite) * fav_get_count();
    s_favorites = malloc(buffer_size);
    if (!persist_exists(SETTING_KEY_FAVORITES) ||
        persist_read_data(SETTING_KEY_FAVORITES, s_favorites, buffer_size) == E_DOES_NOT_EXIST) {
        favorites_deinit();
    }
}

void favorites_deinit() {
    NULL_FREE(s_favorites);
}

size_t fav_get_count() {
    return persist_read_int(SETTING_KEY_FAVORITES_COUNT);
}

Favorite fav_at_index(size_t index) {
    return s_favorites[index];
}

bool fav_add(StationIndex from, StationIndex to) {
    // Check if exists
    if (fav_exists((Favorite){from, to})) {
        return false;
    }
    
    // If s_favorites hasn't been loaded
    if (s_favorites == NULL) {
        favorites_init();
    }
    
    size_t fav_count = fav_get_count();
    // If we already have max count of favorites
    if (fav_count >= FAV_MAX_COUNT) {
        return false;
    } else {
        fav_count += 1;
    }
    
    if (s_favorites == NULL) {
        // The 1st time
        s_favorites = malloc(sizeof(Favorite));
    } else {
        // Resize the existing array
        s_favorites = realloc(s_favorites, sizeof(Favorite) * fav_count);
    }
    
    // If failed to allocate memory
    if (s_favorites == NULL) {
        return false;
    }
    
    // Copy values
    Favorite *new_fav = &s_favorites[fav_count - 1];
    new_fav->from = from;
    new_fav->to = (to == from)?STATION_NON:to;
    
    // Save data
    persist_write_int(SETTING_KEY_FAVORITES_COUNT, fav_count);
    storage_set_favorites(s_favorites, fav_count);
    
    return true;
}

bool fav_delete_at_index(size_t index) {
    if (index >= fav_get_count()) {
        return false;
    }
    size_t new_fav_count = fav_get_count() - 1;
    if (index != new_fav_count) { // If the deleted index wasn't the last one (last_one = count - 1)
        for (size_t i = index; i < new_fav_count; ++i) {
            favorite_copy(&s_favorites[i], &s_favorites[i+1]);
        }
    }
    
    // Save new count
    persist_write_int(SETTING_KEY_FAVORITES_COUNT, new_fav_count);
    
    // Save new favorites list
    s_favorites = realloc(s_favorites, sizeof(Favorite) * new_fav_count);
    storage_set_favorites(s_favorites, new_fav_count);
    return true;
}

bool fav_move_up_index(size_t index) {
    if (index == 0 || index >= fav_get_count()) {
        return false;
    }
    Favorite fav_up = s_favorites[index - 1];
    s_favorites[index - 1] = s_favorites[index];
    s_favorites[index] = fav_up;
    storage_set_favorites(s_favorites, fav_get_count());
    return true;
}

bool fav_exists(Favorite i_fav) {
    for (size_t i = 0; i < fav_get_count(); ++i) {
        Favorite fav = fav_at_index(i);
        if (fav.from == i_fav.from && fav.to == i_fav.to) {
            return true;
        }
    }
    return false;
}