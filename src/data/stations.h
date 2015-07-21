//
//  stations.h
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define STATION_NAME_MAX_LENGTH 100 // We assume all the names are shorter than 100 characters.

#define STATION_CODE_LENGTH 3
#define STATION_NON 999

void stations_init();
void stations_deinit();

size_t stations_get_name(size_t index, char *buffer, const size_t buffer_size);
size_t stations_get_code(size_t index, char *buffer, const size_t buffer_size);