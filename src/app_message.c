//
//  app_message.c
//  PebbleTransilien
//
//  Created by CocoaBob on 19/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static MessageCallbacks s_callbacks;

// MARK: Handlers

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
    if (s_callbacks.message_succeeded_callback != NULL) {
        s_callbacks.message_succeeded_callback(received, context);
    }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
    if (s_callbacks.message_failed_callback != NULL) {
        s_callbacks.message_failed_callback(context);
    }
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    if (s_callbacks.message_failed_callback != NULL) {
        s_callbacks.message_failed_callback(context);
    }
}

// MARK: Setup

void message_init() {
    // Register AppMessage handlers
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    
    // app_message_open should be called before registering handlers
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void message_deinit() {
    app_message_deregister_callbacks();
}

void message_send(DictionaryIterator *parameters,
                  MessageCallbacks callbacks,
                  void *context) {
    s_callbacks = callbacks;
    app_message_set_context(context);
    
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    Tuple *tuple = dict_read_first(parameters);
    while (tuple) {
        dict_write_data(iter, tuple->key, tuple->value->data, tuple->length);
        tuple = dict_read_next(parameters);
    }
    dict_write_end(iter);
    
    app_message_outbox_send();
}

void message_clear_callbacks() {
    s_callbacks.message_succeeded_callback = NULL;
    s_callbacks.message_failed_callback = NULL;
}