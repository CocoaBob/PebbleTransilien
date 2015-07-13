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

size_t size_of_Favorite() {
    static size_t _favorite_size = sizeof(Favorite);
    return _favorite_size;
}

void copy_favorite(Favorite *dest, Favorite *src) {
    strncpy(dest->from, src->from, FAV_COMPONENT_LENGTH);
    strncpy(dest->to, src->to, FAV_COMPONENT_LENGTH);
}

void load_favorites() {
    if (s_favorites != NULL) {
        return;
    }
    size_t buffer_size = size_of_Favorite() * fav_get_count();
    s_favorites = malloc(buffer_size);
    if (!storage_get_favorites(s_favorites,buffer_size)) {
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
        s_favorites = malloc(size_of_Favorite() * 1);
    } else {
        // Resize the existing array
        Favorite* new_favorites = realloc(s_favorites, size_of_Favorite() * fav_count);
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
    strncpy(new_fav->from, from, FAV_COMPONENT_LENGTH);
    strncpy(new_fav->to, to, FAV_COMPONENT_LENGTH);
    
    // Save data
    storage_set_favorites_count(fav_count);
    storage_set_favorites(s_favorites, fav_count);
    
    return true;
}

bool fav_delete_at_index(int16_t index) {
    int16_t new_fav_count = fav_get_count() - 1;
    if (index != new_fav_count) {
        for (int16_t i = index; i < new_fav_count; ++i) {
            copy_favorite(&s_favorites[i], &s_favorites[i+1]);
        }
    }
    Favorite* reduced_favorites = realloc(s_favorites, size_of_Favorite() * new_fav_count);
    if (reduced_favorites != NULL) {
        s_favorites = reduced_favorites;
        
        // Save data
        storage_set_favorites_count(new_fav_count);
        storage_set_favorites(s_favorites, new_fav_count);
        return true;
    }
    return false;
}