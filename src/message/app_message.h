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
    MESSAGE_KEY_TYPE = 100,
    MESSAGE_KEY_CODE_FROM,
    MESSAGE_KEY_CODE_TO,
    MESSAGE_KEY_TRAIN_NUMBER,
    MESSAGE_KEY_NEXT_TRAINS = 200,
    MESSAGE_KEY_TRAIN_DETAILS
} MESSAGE_KEY;

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