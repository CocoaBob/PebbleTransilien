//
//  data_process.c
//  PebbleTransilien
//
//  Created by CocoaBob on 25/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

long relative_time(long timestamp) {
    long o_time = timestamp;
    time_t time_curr = time(NULL); // Contains time zone for Aplite, UTC for Basalt
    o_time = timestamp - time_curr; // UTC time
    // Round to minutes (Remove seconds)
    if (o_time < 0) {
        o_time = -o_time;
        time_t mod = o_time % 60;
        if (mod > 0) {
            o_time -= mod;
        }
    } else {
        time_t mod = o_time % 60;
        if (mod > 0) {
            o_time += (60 - mod);
        }
    }
    
    if (o_time != 0 && timestamp < time_curr) {
        o_time = -o_time;
    }
    o_time /= 60;
    o_time = MAX(-99, MIN(99, o_time));
    return o_time;
}

void time_2_str(time_t timestamp,
                char *o_str,
                size_t o_str_size
#if RELATIVE_TIME_IS_ENABLED
                ,
                bool is_relative_to_now
#endif
                ) {
    // 0 means no time value in the response
    if (timestamp == 0) {
        o_str = '\0';
        return;
    }
    
#if RELATIVE_TIME_IS_ENABLED
    if (is_relative_to_now) {
        snprintf(o_str, o_str_size, "%ldmin", relative_time((long)timestamp));
    } else {
        strftime(o_str, o_str_size, "%H:%M", localtime(&timestamp)); // Show local time
    }
#else
    strftime(o_str, o_str_size, "%H:%M", localtime(&timestamp));
#endif
}

bool string_contains_sub_string(char *string_a, size_t size_a, char *string_b, size_t size_b) {
    if (size_a < size_b) {
        return false;
    }
    
    for (size_t offset = 0; offset <= size_a - size_b; ++offset) {
        if (strncmp(string_a + offset, string_b, size_b) == 0) {
            return true;
        }
    }
    
    return false;
}