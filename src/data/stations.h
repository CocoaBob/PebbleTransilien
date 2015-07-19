//
//  stations.h
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define LOW_MEMORY_MODE

#ifdef LOW_MEMORY_MODE
#define STATION_NAME_MAX_LENGTH 100 // We assume all the names are shorter than 100 characters.
#endif

#define STATION_CODE_LENGTH 3
#define STATION_NON 999

void stations_init();
void stations_deinit();

#ifdef LOW_MEMORY_MODE
size_t stations_get_name(size_t index, char *buffer, const size_t buffer_size);
size_t stations_get_code(size_t index, char *buffer, const size_t buffer_size);
#else
const char* stations_get_name(size_t index);
const char* stations_get_code(size_t index);
#endif