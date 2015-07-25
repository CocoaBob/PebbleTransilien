//
//  data_process.c
//  PebbleTransilien
//
//  Created by CocoaBob on 25/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

void time_2_str(time_t timestamp, char *str, size_t str_size, bool is_relative_to_now) {
    if (is_relative_to_now) {
        time_t time_curr = time(NULL); // Contains time zone for Aplite, UTC for Basalt
        timestamp = timestamp - time_curr; // UTC time
        if (timestamp % 60 > 0) {
            timestamp = ((timestamp / 60) + 1) * 60;
        }
    }
#ifdef PBL_PLATFORM_APLITE
    strftime(str, str_size, "%H:%M", localtime(&timestamp));
#else
    if (is_relative_to_now) {
        strftime(str, str_size, "%H:%M", gmtime(&timestamp)); // Show UTC time
    } else {
        strftime(str, str_size, "%H:%M", localtime(&timestamp)); // Show local time
    }
#endif
}