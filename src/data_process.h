//
//  data_process.h
//  PebbleTransilien
//
//  Created by CocoaBob on 25/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

long relative_time(long timestamp);

//! Convert timestamp to string in format HH:MM
//! @param time Timestamp with time zone offset for Aplite, timestamp of UTC for Basalt
void time_2_str(time_t timestamp,
                char *str,
                size_t str_size
#if RELATIVE_TIME_IS_ENABLED
                ,
                bool is_relative_to_now
#endif
);

//! Check if string_a contains string_b, case-sensitive.
//! @param string Null-terminated string
//! @param sub_string Null-terminated string
bool string_contains_sub_string(char *string_a, size_t size_a, char *string_b, size_t size_b);

#if PBL_ROUND
//! Calculate menu layer row x inset for Pebble Time Round
int16_t get_round_border_x_radius_90(int16_t y);
int16_t get_round_border_x_radius_82(int16_t y);
#endif