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

static FavoriteRequestCallback s_next_trains_request_callback;
static AppTimer *s_next_trains_request_timer;
static size_t s_next_trains_request_index;
static Tuple *s_current_next_trains_dict_tuple;
static DictionaryIterator s_next_trains_dict;
static uint8_t *s_next_trains_dict_buffer;
static uint8_t s_next_trains_dict_buffer_size;

#define KEY_OF_FAV(x) ((x.from << 16) + x.to)

// Request next trains for favorites

static void fav_request_next_trains(void *context);

DataModelNextTrainFavorite *fav_get_next_trains_from_tuple(Tuple *tuple) {
    if (tuple) {
        DataModelNextTrainFavorite *data;
        memcpy(&data, tuple->value->data, sizeof(DataModelNextTrainFavorite *));
        return data;
    }
    return NULL;
}

DataModelNextTrainFavorite *fav_get_next_trains(Favorite favorite) {
    Tuple *tuple = dict_find(&s_next_trains_dict, KEY_OF_FAV(favorite));
    return fav_get_next_trains_from_tuple(tuple);
}

static void message_callback(bool succeeded, void *context, MESSAGE_TYPE type, ...) {
    if (succeeded && type == MESSAGE_TYPE_FAVORITE) {
        va_list ap;
        va_start(ap, type);
        
        if (s_current_next_trains_dict_tuple) {
            DataModelNextTrainFavorite *next_train_favorites = fav_get_next_trains_from_tuple(s_current_next_trains_dict_tuple);
            NULL_FREE(next_train_favorites);
            next_train_favorites = va_arg(ap, void *);
            memcpy(s_current_next_trains_dict_tuple->value->data, &next_train_favorites, sizeof(DataModelNextTrainFavorite *));
        }
        
        va_end(ap);
        
        s_next_trains_request_callback(context);
    }
    
    // Continue the request loop
    fav_request_next_trains(context);
}

static void fav_request_next_trains(void *context) {
    s_next_trains_request_timer = NULL;
    
    // On launch, or no network
    if (!message_is_ready()) {
        s_next_trains_request_index = 0;
        s_next_trains_request_timer = app_timer_register(1000, fav_request_next_trains, context);
        return;
    }
    
    // Request next favorite immediately
    if (s_next_trains_request_index < fav_get_count()) {
        Favorite favorite = fav_at_index(s_next_trains_request_index);
        ++s_next_trains_request_index;
        s_current_next_trains_dict_tuple = dict_find(&s_next_trains_dict, KEY_OF_FAV(favorite));
        
        long key = KEY_OF_FAV(favorite);
        
        message_send(MESSAGE_TYPE_FAVORITE,
                     (MessageCallback)message_callback,
                     context,
                     favorite.from,
                     favorite.to);
    }
    // Wait for 15 seconds to start over again
    else {
        if (s_next_trains_request_index > 0) {
            // Feedback
            vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t[]) {50}, .num_segments = 1});
        }
        
        s_next_trains_request_index = 0;
        s_next_trains_request_timer = app_timer_register(15000, fav_request_next_trains, context);
    }
}

static void fav_release_next_trains_values() {
    Tuple *tuple = dict_read_first(&s_next_trains_dict);
    while (tuple) {
        DataModelNextTrainFavorite *next_train_favorites = fav_get_next_trains_from_tuple(tuple);
        NULL_FREE(next_train_favorites);
        memset(tuple->value->data, 0, sizeof(DataModelNextTrainFavorite *));
        tuple = dict_read_next(&s_next_trains_dict);
    }
}

static void fav_reset_next_trains_dict_buffer() {
    fav_release_next_trains_values();
    // Calculate new dict size
    size_t fav_count = fav_get_count();
    s_next_trains_dict_buffer_size = dict_calc_buffer_size(fav_count,sizeof(DataModelNextTrainFavorite *)*fav_count);
    // Realloc the memory
    s_next_trains_dict_buffer = s_next_trains_dict_buffer?realloc(s_next_trains_dict_buffer, s_next_trains_dict_buffer_size):malloc(s_next_trains_dict_buffer_size);
    
    // Empty old data
    memset(s_next_trains_dict_buffer, 0, s_next_trains_dict_buffer_size);
    
    // Fill with empty data
    void *temp_data = calloc(1, sizeof(DataModelNextTrainFavorite *));
    dict_write_begin(&s_next_trains_dict, s_next_trains_dict_buffer, s_next_trains_dict_buffer_size);
    for (size_t i = 0 ; i < fav_get_count(); ++i) {
        dict_write_data(&s_next_trains_dict, KEY_OF_FAV(fav_at_index(i)), temp_data, sizeof(DataModelNextTrainFavorite *));
    }
    dict_write_end(&s_next_trains_dict);
    free(temp_data);
}

void fav_start_requests(FavoriteRequestCallback callback, void *context) {
    s_next_trains_request_callback = callback;
    fav_request_next_trains(context);
}

void fav_stop_requests() {
    if (s_next_trains_request_timer) {
        app_timer_cancel(s_next_trains_request_timer);
        s_next_trains_request_timer = NULL;
    }
    message_clear_callbacks();
    fav_release_next_trains_values();
    s_next_trains_request_index = 0;
}

// Routines of Favorites

static void storage_set_favorites(const Favorite *favorites, size_t fav_count) {
    if (fav_count == 0) {
        persist_delete(SETTING_KEY_FAVORITES);
    } else {
        persist_write_data(SETTING_KEY_FAVORITES, favorites, sizeof(Favorite) * fav_count);
    }
}

static void fav_copy(Favorite *dest, Favorite *src) {
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
    fav_reset_next_trains_dict_buffer();
}

void favorites_deinit() {
    NULL_FREE(s_favorites);
    
    fav_release_next_trains_values();
    NULL_FREE(s_next_trains_dict_buffer);
    
    if (s_next_trains_request_timer) {
        app_timer_cancel(s_next_trains_request_timer);
        s_next_trains_request_timer = NULL;
    }
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
    
    // Reset request trains dict
    fav_reset_next_trains_dict_buffer();
    
    return true;
}

bool fav_delete_at_index(size_t index) {
    if (index >= fav_get_count()) {
        return false;
    }
    size_t new_fav_count = fav_get_count() - 1;
    if (index != new_fav_count) { // If the deleted index wasn't the last one (last_one = count - 1)
        for (size_t i = index; i < new_fav_count; ++i) {
            fav_copy(&s_favorites[i], &s_favorites[i+1]);
        }
    }
    
    // Save new count
    persist_write_int(SETTING_KEY_FAVORITES_COUNT, new_fav_count);
    
    // Save new favorites list
    s_favorites = realloc(s_favorites, sizeof(Favorite) * new_fav_count);
    storage_set_favorites(s_favorites, new_fav_count);
    
    // Reset request trains dict
    fav_reset_next_trains_dict_buffer();
    
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