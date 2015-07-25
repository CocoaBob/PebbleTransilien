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
    time_t o_time = timestamp;
    size_t offset = 0;
    if (is_relative_to_now) {
        time_t time_curr = time(NULL); // Contains time zone for Aplite, UTC for Basalt
        if (timestamp >= time_curr) {
            o_time = timestamp - time_curr; // UTC time
        } else {
            o_time = time_curr - timestamp; // UTC time
            snprintf(o_str, 2, "-");
            offset = 1;
        }
        if (o_time % 60 > 0) {
            o_time = ((o_time / 60) + 1) * 60;
        }
    }
#ifdef PBL_PLATFORM_APLITE
    strftime(o_str+offset, o_str_size-offset, "%H:%M", localtime(&o_time));
#else
    if (is_relative_to_now) {
        strftime(o_str+offset, o_str_size-offset, "%H:%M", gmtime(&o_time)); // Show UTC time
    } else {
        strftime(o_str+offset, o_str_size-offset, "%H:%M", localtime(&o_time)); // Show local time
    }
#endif
}