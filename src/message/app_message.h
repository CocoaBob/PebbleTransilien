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
    MESSAGE_KEY_TYPE = 1
} MESSAGE_KEY;

typedef void (*MessageSucceededCallback)(DictionaryIterator *received);
typedef void (*MessageFailedCallback)(void);

void message_init();
void message_deinit();

void send_message(MESSAGE_TYPE type,
                  DictionaryIterator *parameters,
                  MessageSucceededCallback succeeded_callback,
                  MessageFailedCallback failed_callback);