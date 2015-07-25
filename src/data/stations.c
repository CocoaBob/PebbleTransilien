//
//  stations.c
//  PebbleTransilien
//
//  Created by CocoaBob on 18/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

ResHandle s_code_handle;
ResHandle s_name_handle;
ResHandle s_name_pos_handle;

void stations_init() {
    s_code_handle = resource_get_handle(RESOURCE_ID_STATION_CODE);
    s_name_handle = resource_get_handle(RESOURCE_ID_STATION_NAME);
    s_name_pos_handle = resource_get_handle(RESOURCE_ID_STATION_NAME_POS);
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