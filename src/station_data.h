//
//  station_data.h
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define STATION_CODE_LENGTH 3
#define STATION_NON 999

void station_data_init();
void station_data_deinit();

const char* station_name_at_index(size_t index);
const char* station_code_at_index(size_t index);