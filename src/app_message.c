//
//  app_message.c
//  PebbleTransilien
//
//  Created by CocoaBob on 19/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

typedef struct {
    MESSAGE_TYPE type;
    MessageCallback callback;
    void *context;
    bool is_ready;
} AppMessage;

// MARK: Handlers

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, AppMessage *user_info) {
    user_info->is_ready = true;
    
    if (!user_info->callback) {
        return;
    }
    
    Tuple *tuple_type = dict_find(received, MESSAGE_KEY_RESPONSE_TYPE);
    if (!tuple_type || tuple_type->value->int8 != user_info->type) {
        return;
    }
    
    Tuple *tuple_payload_count = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT);
    if (!tuple_payload_count) {
        return;
    }
    
    size_t count = tuple_payload_count->value->int16;
    if (count == 0) {
        return;
    }
    
    void *results = NULL;
    
    if (user_info->type == MESSAGE_TYPE_FAVORITE) {
        count = MIN(2, count);
        DataModelNextTrainFavorite *next_trains_simple_list = calloc(2, sizeof(DataModelNextTrainFavorite));
        
        for (size_t idx = 0; idx < count; ++idx) {
            Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + idx);
            if (tuple_payload && tuple_payload->type == TUPLE_BYTE_ARRAY) {
                uint8_t *data = tuple_payload->value->data;
                uint16_t size_left = tuple_payload->length;
                size_t str_length = 0,offset = 0;
                for (size_t data_index = 0; data_index < NEXT_TRAIN_RESPONSE_KEY_COUNT && size_left > 0; ++data_index) {
                    data += offset;
                    str_length = (data_index == NEXT_TRAIN_RESPONSE_KEY_HOUR)?4:strlen((char *)data);
                    offset = str_length + 1;
                    
                    // Interger data
                    if (data_index == NEXT_TRAIN_RESPONSE_KEY_HOUR) {
                        long temp_int = 0;
                        for (size_t i = 0; i < str_length; ++i) {
                            temp_int += data[i] << (8 * (str_length - i - 1));
                        }
                        next_trains_simple_list[idx].hour = temp_int;
                    }
                    // C string data
                    else if (data_index == NEXT_TRAIN_RESPONSE_KEY_PLATFORM) {
                        strncpy(next_trains_simple_list[idx].platform, (char *)data, MIN(2, offset));
                    } else if (data_index == NEXT_TRAIN_RESPONSE_KEY_MENTION) {
                        next_trains_simple_list[idx].mentioned = (str_length > 0);
                    }
                    
                    size_left -= (uint16_t)offset;
                }
            }
        }

        results = next_trains_simple_list;
    }
    else if (user_info->type == MESSAGE_TYPE_NEXT_TRAINS) {
        DataModelNextTrain *next_trains_list = malloc(sizeof(DataModelNextTrain) * count);
        
        for (size_t idx = 0; idx < count; ++idx) {
            Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + idx);
            if (tuple_payload && tuple_payload->type == TUPLE_BYTE_ARRAY) {
                uint8_t *data = tuple_payload->value->data;
                uint16_t size_left = tuple_payload->length;
                size_t str_length = 0,offset = 0;
                for (size_t data_index = 0; data_index < NEXT_TRAIN_RESPONSE_KEY_COUNT && size_left > 0; ++data_index) {
                    data += offset;
                    str_length = (data_index == NEXT_TRAIN_RESPONSE_KEY_HOUR)?4:strlen((char *)data);
                    offset = str_length + 1;
                    
                    // Interger data
                    if (data_index == NEXT_TRAIN_RESPONSE_KEY_HOUR ||
                        data_index == NEXT_TRAIN_RESPONSE_KEY_TERMINUS) {
                        long temp_int = 0;
                        for (size_t i = 0; i < str_length; ++i) {
                            temp_int += data[i] << (8 * (str_length - i - 1));
                        }
                        if (data_index == NEXT_TRAIN_RESPONSE_KEY_HOUR) {
                            next_trains_list[idx].hour = temp_int;
                        } else if (data_index == NEXT_TRAIN_RESPONSE_KEY_TERMINUS) {
                            next_trains_list[idx].terminus = temp_int;
                        }
                    }
                    // C string data
                    else {
                        char *string = calloc(offset, sizeof(char));
                        strncpy(string, (char *)data, offset);
                        if (data_index == NEXT_TRAIN_RESPONSE_KEY_CODE) {
                            next_trains_list[idx].code = string;
                        } else if (data_index == NEXT_TRAIN_RESPONSE_KEY_PLATFORM) {
                            next_trains_list[idx].platform = string;
                        } else if (data_index == NEXT_TRAIN_RESPONSE_KEY_NUMBER) {
                            next_trains_list[idx].number = string;
                        } else if (data_index == NEXT_TRAIN_RESPONSE_KEY_MENTION) {
                            next_trains_list[idx].mention = string;
                        }
                    }
                    
                    size_left -= (uint16_t)offset;
                }
            }
        }
        
        results = next_trains_list;
    }
    else if (user_info->type == MESSAGE_TYPE_TRAIN_DETAILS) {
        DataModelTrainDetail *train_details_list = malloc(sizeof(DataModelTrainDetail) * count);
        
        for (size_t idx = 0; idx < count; ++idx) {
            Tuple *tuple_payload = dict_find(received, MESSAGE_KEY_RESPONSE_PAYLOAD + idx);
            if (tuple_payload && tuple_payload->type == TUPLE_BYTE_ARRAY) {
                uint8_t *data = tuple_payload->value->data;
                uint16_t size_left = tuple_payload->length;
                size_t str_length = 0,offset = 0;
                for (size_t data_index = 0; data_index < TRAIN_DETAIL_RESPONSE_KEY_COUNT && size_left > 0; ++data_index) {
                    data += offset;
                    str_length = (data_index == TRAIN_DETAIL_RESPONSE_KEY_TIME)?4:strlen((char *)data);
                    offset = str_length + 1;
                    
                    long temp_int = 0;
                    for (size_t i = 0; i < str_length; ++i) {
                        temp_int += data[i] << (8 * (str_length - i - 1));
                    }
                    if (data_index == TRAIN_DETAIL_RESPONSE_KEY_TIME) {
                        train_details_list[idx].time = temp_int;
                    } else if (data_index == TRAIN_DETAIL_RESPONSE_KEY_STATION) {
                        train_details_list[idx].station = temp_int;
                    }
                    
                    size_left -= (uint16_t)offset;
                }
            }
        }
        
        results = train_details_list;
    }
    
    // Call callback
    user_info->callback(true, user_info->context, user_info->type, results, count);
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, AppMessage *user_info) {
    if (user_info->callback) {
        user_info->callback(false, user_info->context, user_info->type);
    }
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, AppMessage *user_info) {
    if (user_info->callback) {
        user_info->callback(false, user_info->context, user_info->type);
    }
}

// MARK: Setup

void message_init() {
    AppMessage *user_info = calloc(1, sizeof(AppMessage));
    app_message_set_context(user_info);
    
    // Register AppMessage handlers
    app_message_register_inbox_received((AppMessageInboxReceived)in_received_handler);
    app_message_register_inbox_dropped((AppMessageInboxDropped)in_dropped_handler);
    app_message_register_outbox_failed((AppMessageOutboxFailed)out_failed_handler);
    
    // app_message_open should be called before registering handlers
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void message_deinit() {
    app_message_deregister_callbacks();
    
    AppMessage *user_info = app_message_get_context();
    free(user_info);
}

// MARK: Routines

bool message_is_ready() {
    AppMessage *user_info = app_message_get_context();
    return user_info->is_ready && connection_service_peek_pebble_app_connection();
}

// MARK: Send with callbacks

static void write_station_index_to_dict(DictionaryIterator *iter, StationIndex station_index, MESSAGE_KEY message_key) {
    if (station_index != STATION_NON) {
        char *data = malloc(STATION_CODE_LENGTH);
        stations_get_code(station_index, data, STATION_CODE_LENGTH);
        size_t length = strlen(data);
        if (length > STATION_CODE_LENGTH) {
            length = STATION_CODE_LENGTH;
        }
        dict_write_data(iter, message_key, (uint8_t *)data, length);
        free(data);
    }
}

void message_send(MESSAGE_TYPE type, MessageCallback callback, void *context, ...) {
    AppMessage *user_info = app_message_get_context();
    user_info->type = type;
    user_info->callback = callback;
    user_info->context = context;
    
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    va_list ap;
    va_start(ap, context);
    
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_TYPE, type);
    if (type == MESSAGE_TYPE_NEXT_TRAINS ||
        type == MESSAGE_TYPE_FAVORITE) {
        write_station_index_to_dict(iter, va_arg(ap, StationIndex), MESSAGE_KEY_REQUEST_CODE_FROM); // StationIndex for from
        write_station_index_to_dict(iter, va_arg(ap, StationIndex), MESSAGE_KEY_REQUEST_CODE_TO);   // StationIndex for to
    } else if (type == MESSAGE_TYPE_TRAIN_DETAILS) {
        dict_write_data(iter, MESSAGE_KEY_REQUEST_TRAIN_NUMBER, va_arg(ap, uint8_t *), va_arg(ap, size_t)); // Train Number String & Train Number String Length
    }
    
    va_end(ap);
    
    dict_write_end(iter);
    
    app_message_outbox_send();
}

void message_clear_callbacks() {
    AppMessage *user_info = app_message_get_context();
    user_info->callback = NULL;
}