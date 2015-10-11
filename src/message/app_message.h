//
//  app_message.h
//  PebbleTransilien
//
//  Created by CocoaBob on 19/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

typedef enum {
    MESSAGE_TYPE_NEXT_TRAINS = 0,
    MESSAGE_TYPE_TRAIN_DETAILS
} MESSAGE_TYPE;

typedef enum {
    MESSAGE_KEY_REQUEST_TYPE = 100,
    MESSAGE_KEY_REQUEST_CODE_FROM,
    MESSAGE_KEY_REQUEST_CODE_TO,
    MESSAGE_KEY_REQUEST_TRAIN_NUMBER,
    MESSAGE_KEY_RESPONSE_TYPE = 200,
    MESSAGE_KEY_RESPONSE_PAYLOAD_COUNT,
    MESSAGE_KEY_RESPONSE_PAYLOAD = 1000,
} MESSAGE_KEY;

typedef enum {
    NEXT_TRAIN_KEY_CODE,
    NEXT_TRAIN_KEY_HOUR,
    NEXT_TRAIN_KEY_PLATFORM,
    NEXT_TRAIN_KEY_TERMINUS,
    NEXT_TRAIN_KEY_NUMBER,
    NEXT_TRAIN_KEY_MENTION,
    NEXT_TRAIN_KEY_COUNT
} NEXT_TRAIN_KEY;

typedef enum {
    TRAIN_DETAIL_KEY_TIME,
    TRAIN_DETAIL_KEY_STATION,
    TRAIN_DETAIL_KEY_COUNT
} TRAIN_DETAIL_KEY;

typedef void (*MessageSucceededCallback)(DictionaryIterator *received);
typedef void (*MessageFailedCallback)(void);

typedef struct MessageCallbacks {
    MessageSucceededCallback message_succeeded_callback;
    MessageFailedCallback message_failed_callback;
} MessageCallbacks;

void message_init();
void message_deinit();

void message_send(DictionaryIterator *parameters,
                  MessageCallbacks callbacks);
void message_clear_callbacks();