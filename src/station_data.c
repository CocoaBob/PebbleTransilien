//
//  station_data.c
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

#ifdef LOW_MEMORY_MODE

ResHandle s_code_handle;
ResHandle s_name_handle;
static size_t* s_name_positions;

void station_data_init() {
    // Get station codes
    s_code_handle = resource_get_handle(RESOURCE_ID_STATION_CODE);
    size_t code_size = resource_size(s_code_handle);
    
    // Get station names
    s_name_handle = resource_get_handle(RESOURCE_ID_STATION_NAME);
    
    // Build name indexes
    size_t count = code_size / STATION_CODE_LENGTH;
    s_name_positions = malloc(sizeof(size_t) * count);
    size_t position = 0;
    char *buffer = malloc(sizeof(char) * STATION_NAME_MAX_LENGTH);
    for (size_t index = 0; index < count; ++index) {
        resource_load_byte_range(s_name_handle, position, (uint8_t *)buffer, STATION_NAME_MAX_LENGTH);
        
        size_t name_length = strlen(buffer) + 1;
        s_name_positions[index] = position;
        
        position += name_length;
    }
    free(buffer);
}

void station_data_deinit() {
    free(s_name_positions);
}

size_t station_data_get_name(size_t index, char *buffer, const size_t buffer_size) {
    size_t position = s_name_positions[index];
    return resource_load_byte_range(s_name_handle, position, (uint8_t *)buffer, buffer_size);
}

size_t station_data_get_code(size_t index, char *buffer, const size_t buffer_size) {
    return resource_load_byte_range(s_code_handle, STATION_CODE_LENGTH * index, (uint8_t *)buffer, buffer_size);
}

#else

static char* s_codes;
static char* s_names;
static char** s_name_pointers;

void station_data_init() {
    // Load station codes
    ResHandle code_handle = resource_get_handle(RESOURCE_ID_STATION_CODE);
    size_t code_size = resource_size(code_handle);
    s_codes = malloc(code_size);
    resource_load(code_handle, (uint8_t *)s_codes, code_size);
    
    // Load station names
    ResHandle name_handle = resource_get_handle(RESOURCE_ID_STATION_NAME);
    size_t name_size = resource_size(name_handle);
    s_names = malloc(name_size);
    resource_load(name_handle, (uint8_t *)s_names, name_size);
    
    // Build name indexes
    size_t count = code_size / STATION_CODE_LENGTH;
    s_name_pointers = malloc(sizeof(char*) * count);
    char *name = s_names;
    for (size_t index = 0; index < count; ++index) {
        s_name_pointers[index] = name;
        name += strlen(name) + 1;
    }
}

void station_data_deinit() {
    free(s_codes);
    free(s_names);
    free(s_name_pointers);
}

const char* station_data_get_name(size_t index) {
    return s_name_pointers[index];
}

const char* station_data_get_code(size_t index) {
    return s_codes + STATION_CODE_LENGTH * index;
}

#endif