//
//  stations.c
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static ResHandle s_code_handle;
static ResHandle s_name_handle;
static ResHandle s_name_pos_handle;
static ResHandle s_name_search_handle;

static size_t s_names_to_search_size;
static char *s_names_to_search;

void stations_init() {
    s_code_handle = resource_get_handle(RESOURCE_ID_STATION_CODE);
    s_name_handle = resource_get_handle(RESOURCE_ID_STATION_NAME);
    s_name_pos_handle = resource_get_handle(RESOURCE_ID_STATION_NAME_POS);
    s_name_search_handle = resource_get_handle(RESOURCE_ID_STATION_NAME_SEARCH);
}

void stations_deinit() {
}

size_t stations_get_name_pos(size_t index) {
    uint8_t pos_bytes[STATION_NAME_POS_VALUE_LENGTH];
    resource_load_byte_range(s_name_pos_handle, STATION_NAME_POS_VALUE_LENGTH * index, pos_bytes, STATION_NAME_POS_VALUE_LENGTH);
    return (pos_bytes[0] << 8) + pos_bytes[1];
}

size_t stations_get_name(size_t index, char *buffer, const size_t buffer_size) {
    size_t name_pos = stations_get_name_pos(index);
    return resource_load_byte_range(s_name_handle, name_pos, (uint8_t *)buffer, buffer_size);
}

size_t stations_get_code(size_t index, char *buffer, const size_t buffer_size) {
    return resource_load_byte_range(s_code_handle, STATION_CODE_LENGTH * index, (uint8_t *)buffer, buffer_size);
}

void stations_search_name_begin() {
    s_names_to_search_size = resource_size(s_name_search_handle);
    s_names_to_search = malloc(s_names_to_search_size);
    resource_load(s_name_search_handle, (uint8_t *)s_names_to_search, s_names_to_search_size);
}

void stations_search_name(char *search_string, size_t *buffer, size_t buffer_size, size_t *result_size) {
    size_t index_result = 0;    // Index of the results array
    size_t index_searching = 0; // Index of all the names to search
    size_t search_string_offset = 0;
    size_t size_of_search_string = strlen(search_string);
    while (search_string_offset < s_names_to_search_size && index_result < buffer_size) {
        char *names_to_search = s_names_to_search + search_string_offset;
        size_t name_length = strlen(names_to_search);
        if (string_contains_sub_string(names_to_search, name_length, search_string, size_of_search_string)) {
            buffer[index_result] = index_searching;
            ++index_result;
        }
        ++index_searching;
        search_string_offset += name_length + 1;
    }
    *result_size = index_result;
}

void stations_search_name_end() {
    NULL_FREE(s_names_to_search);
}