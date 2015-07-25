//
//  data_process.h
//  PebbleTransilien
//
//  Created by CocoaBob on 25/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

//! Convert timestamp to string in format HH:MM
//! @param time Timestamp with time zone offset for Aplite, timestamp of UTC for Basalt
void time_2_str(time_t timestamp, char *str, size_t str_size, bool is_relative_to_now);