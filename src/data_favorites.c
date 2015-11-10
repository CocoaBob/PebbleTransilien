//
//  data_favorites.c
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

typedef struct {
    Favorite *favorites;
#if MINI_TIMETABLE_IS_ENABLED
    MiniTimetableRequestCallback mini_timetable_request_callback;
    AppTimer *mini_timetable_request_timer;
    size_t mini_timetable_request_index;
    Tuple *current_mini_timetable_dict_tuple;
    DictionaryIterator mini_timetable_dict;
    uint8_t *mini_timetable_dict_buffer;
    uint8_t mini_timetable_dict_buffer_size;
#endif
} FavoriteData;

static FavoriteData *s_favorite_data;

#if MINI_TIMETABLE_IS_ENABLED

#define KEY_OF_FAV(x) ((x.from << 16) + x.to)

// Request next trains for favorites

static void fav_request_mini_timetable(void *context);

DataModelMiniTimetable *fav_get_mini_timetable_from_tuple(Tuple *tuple) {
    if (tuple) {
        DataModelMiniTimetable *data;
        memcpy(&data, tuple->value->data, sizeof(DataModelMiniTimetable *));
        return data;
    }
    return NULL;
}

DataModelMiniTimetable *fav_get_mini_timetables(Favorite favorite) {
    Tuple *tuple = dict_find(&s_favorite_data->mini_timetable_dict, KEY_OF_FAV(favorite));
    return fav_get_mini_timetable_from_tuple(tuple);
}

static void message_callback(bool succeeded, void *context, MESSAGE_TYPE type, ...) {
    if (succeeded && type == MESSAGE_TYPE_MINI_TIMETABLE) {
        va_list ap;
        va_start(ap, type);
        
        if (s_favorite_data->current_mini_timetable_dict_tuple) {
            DataModelMiniTimetable *mini_timetables = fav_get_mini_timetable_from_tuple(s_favorite_data->current_mini_timetable_dict_tuple);
            NULL_FREE(mini_timetables);
            mini_timetables = va_arg(ap, void *);
            memcpy(s_favorite_data->current_mini_timetable_dict_tuple->value->data, &mini_timetables, sizeof(DataModelMiniTimetable *));
        }
        
        va_end(ap);
        
        s_favorite_data->mini_timetable_request_callback(context);
    }
    
    // Continue the request loop
    fav_request_mini_timetable(context);
}

static void fav_request_mini_timetable(void *context) {
    s_favorite_data->mini_timetable_request_timer = NULL;
    
    // On launch, or no network
    if (!message_is_ready()) {
        s_favorite_data->mini_timetable_request_index = 0;
        s_favorite_data->mini_timetable_request_timer = app_timer_register(1000, fav_request_mini_timetable, context);
        return;
    }
    
    // Request next favorite immediately
    if (s_favorite_data->mini_timetable_request_index < fav_get_count()) {
        Favorite favorite = fav_at_index(s_favorite_data->mini_timetable_request_index);
        ++s_favorite_data->mini_timetable_request_index;
        s_favorite_data->current_mini_timetable_dict_tuple = dict_find(&s_favorite_data->mini_timetable_dict, KEY_OF_FAV(favorite));
        
        message_send(MESSAGE_TYPE_MINI_TIMETABLE,
                     (MessageCallback)message_callback,
                     context,
                     favorite.from,
                     favorite.to);
    }
    // Wait for 15 seconds to start over again
    else {
        if (s_favorite_data->mini_timetable_request_index > 0) {
            // Feedback
            vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t[]) {50}, .num_segments = 1});
        }
        
        s_favorite_data->mini_timetable_request_index = 0;
        s_favorite_data->mini_timetable_request_timer = app_timer_register(15000, fav_request_mini_timetable, context);
    }
}

static void fav_release_mini_timetable_values() {
    Tuple *tuple = dict_read_first(&s_favorite_data->mini_timetable_dict);
    while (tuple) {
        DataModelMiniTimetable *mini_timetables = fav_get_mini_timetable_from_tuple(tuple);
        NULL_FREE(mini_timetables);
        memset(tuple->value->data, 0, sizeof(DataModelMiniTimetable *));
        tuple = dict_read_next(&s_favorite_data->mini_timetable_dict);
    }
}

static void fav_reset_mini_timetable_dict_buffer() {
    fav_release_mini_timetable_values();
    // Calculate new dict size
    size_t fav_count = fav_get_count();
    s_favorite_data->mini_timetable_dict_buffer_size = dict_calc_buffer_size(fav_count,sizeof(DataModelMiniTimetable *)*fav_count);
    // Realloc the memory
    s_favorite_data->mini_timetable_dict_buffer = s_favorite_data->mini_timetable_dict_buffer?realloc(s_favorite_data->mini_timetable_dict_buffer, s_favorite_data->mini_timetable_dict_buffer_size):malloc(s_favorite_data->mini_timetable_dict_buffer_size);
    
    // Empty old data
    memset(s_favorite_data->mini_timetable_dict_buffer, 0, s_favorite_data->mini_timetable_dict_buffer_size);
    
    // Fill with empty data
    void *temp_data = calloc(1, sizeof(DataModelMiniTimetable *));
    dict_write_begin(&s_favorite_data->mini_timetable_dict, s_favorite_data->mini_timetable_dict_buffer, s_favorite_data->mini_timetable_dict_buffer_size);
    for (size_t i = 0 ; i < fav_get_count(); ++i) {
        dict_write_data(&s_favorite_data->mini_timetable_dict, KEY_OF_FAV(fav_at_index(i)), temp_data, sizeof(DataModelMiniTimetable *));
    }
    dict_write_end(&s_favorite_data->mini_timetable_dict);
    free(temp_data);
}

void fav_start_requests(MiniTimetableRequestCallback callback, void *context) {
    if (s_favorite_data->mini_timetable_request_callback != callback) {
        s_favorite_data->mini_timetable_request_callback = callback;
        fav_request_mini_timetable(context);
    }
}

void fav_stop_requests() {
    if (s_favorite_data->mini_timetable_request_timer) {
        app_timer_cancel(s_favorite_data->mini_timetable_request_timer);
        s_favorite_data->mini_timetable_request_timer = NULL;
    }
    message_clear_callbacks();
    fav_release_mini_timetable_values();
    s_favorite_data->mini_timetable_request_index = 0;
    s_favorite_data->mini_timetable_request_callback = NULL;
}

#endif

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
    if (s_favorite_data != NULL) {
        return;
    }
    
    s_favorite_data = calloc(1, sizeof(FavoriteData));
    size_t buffer_size = sizeof(Favorite) * fav_get_count();
    s_favorite_data->favorites = malloc(buffer_size);
    if (!persist_exists(SETTING_KEY_FAVORITES) ||
        persist_read_data(SETTING_KEY_FAVORITES, s_favorite_data->favorites, buffer_size) == E_DOES_NOT_EXIST) {
        favorites_deinit();
    }
#if MINI_TIMETABLE_IS_ENABLED
    fav_reset_mini_timetable_dict_buffer();
#endif
}

void favorites_deinit() {
    NULL_FREE(s_favorite_data->favorites);
    
#if MINI_TIMETABLE_IS_ENABLED
    fav_release_mini_timetable_values();
    NULL_FREE(s_favorite_data->mini_timetable_dict_buffer);
    
    if (s_favorite_data->mini_timetable_request_timer) {
        app_timer_cancel(s_favorite_data->mini_timetable_request_timer);
        s_favorite_data->mini_timetable_request_timer = NULL;
    }
#endif
    
    NULL_FREE(s_favorite_data);
}

size_t fav_get_count() {
    return persist_read_int(SETTING_KEY_FAVORITES_COUNT);
}

Favorite fav_at_index(size_t index) {
    return s_favorite_data->favorites[index];
}

bool fav_add(StationIndex from, StationIndex to) {
    // Check if exists
    if (fav_exists((Favorite){from, to})) {
        return false;
    }
    
    // If s_favorite_data->favorites hasn't been loaded
    if (s_favorite_data->favorites == NULL) {
        favorites_init();
    }
    
    size_t fav_count = fav_get_count();
    // If we already have max count of favorites
    if (fav_count >= FAV_MAX_COUNT) {
        return false;
    } else {
        fav_count += 1;
    }
    
    if (s_favorite_data->favorites == NULL) {
        // The 1st time
        s_favorite_data->favorites = malloc(sizeof(Favorite));
    } else {
        // Resize the existing array
        s_favorite_data->favorites = realloc(s_favorite_data->favorites, sizeof(Favorite) * fav_count);
    }
    
    // If failed to allocate memory
    if (s_favorite_data->favorites == NULL) {
        return false;
    }
    
    // Copy values
    Favorite *new_fav = &s_favorite_data->favorites[fav_count - 1];
    new_fav->from = from;
    new_fav->to = (to == from)?STATION_NON:to;
    
    // Save data
    persist_write_int(SETTING_KEY_FAVORITES_COUNT, fav_count);
    storage_set_favorites(s_favorite_data->favorites, fav_count);
    
#if MINI_TIMETABLE_IS_ENABLED
    // Reset request trains dict
    fav_reset_mini_timetable_dict_buffer();
#endif
    
    return true;
}

bool fav_delete_at_index(size_t index) {
    if (index >= fav_get_count()) {
        return false;
    }
    size_t new_fav_count = fav_get_count() - 1;
    if (index != new_fav_count) { // If the deleted index wasn't the last one (last_one = count - 1)
        for (size_t i = index; i < new_fav_count; ++i) {
            fav_copy(&s_favorite_data->favorites[i], &s_favorite_data->favorites[i+1]);
        }
    }
    
    // Save new count
    persist_write_int(SETTING_KEY_FAVORITES_COUNT, new_fav_count);
    
    // Save new favorites list
    s_favorite_data->favorites = realloc(s_favorite_data->favorites, sizeof(Favorite) * new_fav_count);
    storage_set_favorites(s_favorite_data->favorites, new_fav_count);
    
#if MINI_TIMETABLE_IS_ENABLED
    // Reset request trains dict
    fav_reset_mini_timetable_dict_buffer();
#endif
    
    return true;
}

bool fav_move_up_index(size_t index) {
    if (index == 0 || index >= fav_get_count()) {
        return false;
    }
    Favorite fav_up = s_favorite_data->favorites[index - 1];
    s_favorite_data->favorites[index - 1] = s_favorite_data->favorites[index];
    s_favorite_data->favorites[index] = fav_up;
    storage_set_favorites(s_favorite_data->favorites, fav_get_count());
    
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