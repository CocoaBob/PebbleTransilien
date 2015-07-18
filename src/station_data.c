//
//  station_data.c
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

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
    
    // Find all names
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

const char* station_name_at_index(size_t index) {
    return s_name_pointers[index];
}

const char* station_code_at_index(size_t index) {
    return s_codes + STATION_CODE_LENGTH * index;
}