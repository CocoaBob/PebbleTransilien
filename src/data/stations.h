//
//  stations.h
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define STATION_NAME_MAX_LENGTH 100 // We assume that all the names are shorter than 100 characters.

#define STATION_CODE_LENGTH 3
#define STATION_NAME_POS_VALUE_LENGTH 2
#define STATION_NON 999

#define STATION_SEARH_RESULT_MAX_COUNT 10

void stations_init();
void stations_deinit();

size_t stations_get_name(size_t index, char *buffer, const size_t buffer_size);
size_t stations_get_code(size_t index, char *buffer, const size_t buffer_size);

void stations_search_name_begin();
void stations_search_name(char *search_string, size_t *buffer, size_t buffer_size, size_t *result_size);
void stations_search_name_end();