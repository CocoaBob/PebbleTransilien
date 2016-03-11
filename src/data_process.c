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
        long r_time = relative_time((long)timestamp);
        if (-99 <= r_time && r_time <= 99) {
            snprintf(o_str, o_str_size, "%ldmin", r_time);
            return;
        }
    }
#endif
    strftime(o_str, o_str_size, "%H:%M", localtime(&timestamp)); // Show local time
}

bool string_contains_sub_string(char *T, size_t n, char *P, size_t m) {
    if (n < m) {
        return false;
    }
    
    // Bad algorithm
//    for (size_t offset = 0; offset <= n - m; ++offset) {
//        if (strncmp(T + offset, P, m) == 0) {
//            return true;
//        }
//    }
    
    // KMP algorithm
    size_t pi[m];
    pi[0] = 0;
    size_t k = 0;
    size_t q,i;
    /* Compute prefix array pi[] */
    for(q = 2; q <= m; q++) {
        while(k > 0 && P[k] != P[q-1])
            k = pi[k-1];
        if(P[k] == P[q-1])
            k++;
        pi[q-1] = k;
    }
    q = 0;
    for(i = 0; i < n; i++) {
        while(q > 0 && P[q] != T[i])
            q = pi[q-1];
        if(P[q] == T[i])
            q++;
        if(q == m) { // Match!
            return true;
        }
    }
    
    return false;
}

#if PBL_ROUND
// Pythagorean theorem, r^2=x^2+y^2
static int16_t s_round_squares_radius_90[91] = {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,3,3,3,4,4,4,4,5,5,6,6,6,7,7,8,8,8,9,9,10,10,11,11,12,13,13,14,15,15,16,17,17,18,19,20,20,21,22,23,24,25,26,27,28,29,30,31,32,33,35,36,37,39,40,42,43,45,47,49,51,53,55,58,60,63,67,71,77,90};
int16_t get_round_border_x_radius_90(int16_t y) {
    if (y > 180 || y < 0) {
        return 0;
    }
    return s_round_squares_radius_90[ABS(y - 90)];
}
static int16_t s_round_squares_radius_82[83] = {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,3,3,3,4,4,4,5,5,5,6,6,7,7,7,8,8,9,9,10,10,11,12,12,13,13,14,15,16,16,17,18,19,19,20,21,22,23,24,25,26,27,28,30,31,32,33,35,36,38,39,41,43,45,47,49,51,54,57,60,64,69,82};
int16_t get_round_border_x_radius_82(int16_t y) {
    if (y > 164 || y < 0) {
        return 0;
    }
    return s_round_squares_radius_82[ABS(y - 82)];
}
#endif