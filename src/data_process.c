//
//  data_process.c
//  PebbleTransilien
//
//  Created by CocoaBob on 25/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

void time_2_str(time_t timestamp, char *o_str, size_t o_str_size, bool is_relative_to_now) {
    // 0 means no time value in the response
    if (timestamp == 0) {
        o_str = '\0';
        return;
    }
    
#if defined(PBL_PLATFORM_APLITE)
    strftime(o_str, o_str_size, "%H:%M", localtime(&timestamp));
#else
    time_t o_time = timestamp;
    size_t offset = 0;
    if (is_relative_to_now) {
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
            snprintf(o_str, 2, "-");
            offset = 1;
        }
    }
    if (is_relative_to_now) {
        o_time /= 60;
        o_time = MIN(99, o_time);
        snprintf(o_str+offset, o_str_size-offset, "%lldmin", (long long)o_time);
    } else {
        strftime(o_str+offset, o_str_size-offset, "%H:%M", localtime(&o_time)); // Show local time
    }
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