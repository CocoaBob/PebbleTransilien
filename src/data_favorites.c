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

#if MINI_TIMETABLE_IS_ENABLED

static MiniTimetableRequestCallback s_mini_timetable_request_callback;
static AppTimer *s_mini_timetable_request_timer;
static size_t s_mini_timetable_request_index;
static Tuple *s_current_mini_timetable_dict_tuple;
static DictionaryIterator s_mini_timetable_dict;
static uint8_t *s_mini_timetable_dict_buffer;
static uint8_t s_mini_timetable_dict_buffer_size;

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
    Tuple *tuple = dict_find(&s_mini_timetable_dict, KEY_OF_FAV(favorite));
    return fav_get_mini_timetable_from_tuple(tuple);
}

static void message_callback(bool succeeded, void *context, MESSAGE_TYPE type, ...) {
    if (succeeded && type == MESSAGE_TYPE_MINI_TIMETABLE) {
        va_list ap;
        va_start(ap, type);
        
        if (s_current_mini_timetable_dict_tuple) {
            DataModelMiniTimetable *mini_timetables = fav_get_mini_timetable_from_tuple(s_current_mini_timetable_dict_tuple);
            NULL_FREE(mini_timetables);
            mini_timetables = va_arg(ap, void *);
            memcpy(s_current_mini_timetable_dict_tuple->value->data, &mini_timetables, sizeof(DataModelMiniTimetable *));
        }
        
        va_end(ap);
        
        s_mini_timetable_request_callback(context);
    }
    
    // Continue the request loop
    fav_request_mini_timetable(context);
}

static void fav_request_mini_timetable(void *context) {
    s_mini_timetable_request_timer = NULL;
    
    // On launch, or no network
    if (!message_is_ready()) {
        s_mini_timetable_request_index = 0;
        s_mini_timetable_request_timer = app_timer_register(1000, fav_request_mini_timetable, context);
        return;
    }
    
    // Request next favorite immediately
    if (s_mini_timetable_request_index < fav_get_count()) {
        Favorite favorite = fav_at_index(s_mini_timetable_request_index);
        ++s_mini_timetable_request_index;
        s_current_mini_timetable_dict_tuple = dict_find(&s_mini_timetable_dict, KEY_OF_FAV(favorite));
        
        message_send(MESSAGE_TYPE_MINI_TIMETABLE,
                     (MessageCallback)message_callback,
                     context,
                     favorite.from,
                     favorite.to);
    }
    // Wait for 15 seconds to start over again
    else {
        if (s_mini_timetable_request_index > 0) {
            // Feedback
            vibes_enqueue_custom_pattern((VibePattern){.durations = (uint32_t[]) {50}, .num_segments = 1});
        }
        
        s_mini_timetable_request_index = 0;
        s_mini_timetable_request_timer = app_timer_register(15000, fav_request_mini_timetable, context);
    }
}

static void fav_release_mini_timetable_values() {
    Tuple *tuple = dict_read_first(&s_mini_timetable_dict);
    while (tuple) {
        DataModelMiniTimetable *mini_timetables = fav_get_mini_timetable_from_tuple(tuple);
        NULL_FREE(mini_timetables);
        memset(tuple->value->data, 0, sizeof(DataModelMiniTimetable *));
        tuple = dict_read_next(&s_mini_timetable_dict);
    }
}

static void fav_reset_mini_timetable_dict_buffer() {
    fav_release_mini_timetable_values();
    // Calculate new dict size
    size_t fav_count = fav_get_count();
    s_mini_timetable_dict_buffer_size = dict_calc_buffer_size(fav_count,sizeof(DataModelMiniTimetable *)*fav_count);
    // Realloc the memory
    s_mini_timetable_dict_buffer = s_mini_timetable_dict_buffer?realloc(s_mini_timetable_dict_buffer, s_mini_timetable_dict_buffer_size):malloc(s_mini_timetable_dict_buffer_size);
    
    // Empty old data
    memset(s_mini_timetable_dict_buffer, 0, s_mini_timetable_dict_buffer_size);
    
    // Fill with empty data
    void *temp_data = calloc(1, sizeof(DataModelMiniTimetable *));
    dict_write_begin(&s_mini_timetable_dict, s_mini_timetable_dict_buffer, s_mini_timetable_dict_buffer_size);
    for (size_t i = 0 ; i < fav_get_count(); ++i) {
        dict_write_data(&s_mini_timetable_dict, KEY_OF_FAV(fav_at_index(i)), temp_data, sizeof(DataModelMiniTimetable *));
    }
    dict_write_end(&s_mini_timetable_dict);
    free(temp_data);
}

void fav_start_requests(MiniTimetableRequestCallback callback, void *context) {
    if (s_mini_timetable_request_callback != callback) {
        s_mini_timetable_request_callback = callback;
        fav_request_mini_timetable(context);
    }
}

void fav_stop_requests() {
    if (s_mini_timetable_request_timer) {
        app_timer_cancel(s_mini_timetable_request_timer);
        s_mini_timetable_request_timer = NULL;
    }
    message_clear_callbacks();
    fav_release_mini_timetable_values();
    s_mini_timetable_request_index = 0;
    s_mini_timetable_request_callback = NULL;
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
    if (s_favorites != NULL) {
        return;
    }
    size_t buffer_size = sizeof(Favorite) * fav_get_count();
    s_favorites = malloc(buffer_size);
    if (!persist_exists(SETTING_KEY_FAVORITES) ||
        persist_read_data(SETTING_KEY_FAVORITES, s_favorites, buffer_size) == E_DOES_NOT_EXIST) {
        favorites_deinit();
    }
#if MINI_TIMETABLE_IS_ENABLED
    fav_reset_mini_timetable_dict_buffer();
#endif
}

void favorites_deinit() {
    NULL_FREE(s_favorites);
    
#if MINI_TIMETABLE_IS_ENABLED
    fav_release_mini_timetable_values();
    NULL_FREE(s_mini_timetable_dict_buffer);
    
    if (s_mini_timetable_request_timer) {
        app_timer_cancel(s_mini_timetable_request_timer);
        s_mini_timetable_request_timer = NULL;
    }
#endif
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
            fav_copy(&s_favorites[i], &s_favorites[i+1]);
        }
    }
    
    // Save new count
    persist_write_int(SETTING_KEY_FAVORITES_COUNT, new_fav_count);
    
    // Save new favorites list
    s_favorites = realloc(s_favorites, sizeof(Favorite) * new_fav_count);
    storage_set_favorites(s_favorites, new_fav_count);
    
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