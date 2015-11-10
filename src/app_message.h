//
//  app_message.h
//  PebbleTransilien
//
//  Created by CocoaBob on 19/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

typedef enum {
    MESSAGE_TYPE_ERROR = -1,
    MESSAGE_TYPE_MINI_TIMETABLE = 0,
    MESSAGE_TYPE_NEXT_TRAINS,
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
    NEXT_TRAIN_RESPONSE_KEY_CODE,
    NEXT_TRAIN_RESPONSE_KEY_HOUR,
    NEXT_TRAIN_RESPONSE_KEY_PLATFORM,
    NEXT_TRAIN_RESPONSE_KEY_TERMINUS,
    NEXT_TRAIN_RESPONSE_KEY_NUMBER,
    NEXT_TRAIN_RESPONSE_KEY_MENTION,
    NEXT_TRAIN_RESPONSE_KEY_COUNT
} NEXT_TRAIN_RESPONSE_KEY;

typedef enum {
    TRAIN_DETAIL_RESPONSE_KEY_TIME,
    TRAIN_DETAIL_RESPONSE_KEY_STATION,
    TRAIN_DETAIL_RESPONSE_KEY_COUNT
} TRAIN_DETAIL_RESPONSE_KEY;

typedef void (*MessageCallback)(bool succeeded, void *context, MESSAGE_TYPE type, ...);

typedef struct MessageCallbacks {
    MessageCallback message_callback;
} MessageCallbacks;

// MARK: Setup
void message_init();
void message_deinit();

// MARK: Routines
bool message_is_ready();

// MARK: Send with callbacks
void message_send(MESSAGE_TYPE type, MessageCallback callback, void *context, ...);
void message_clear_callbacks();