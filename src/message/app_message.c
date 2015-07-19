//
//  app_message.c
//  PebbleTransilien
//
//  Created by CocoaBob on 19/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static MessageSucceededCallback s_current_succeeded_callback;
static MessageFailedCallback s_current_failed_callback;

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
    s_current_succeeded_callback(received);
    
//	Tuple *tuple;
//	
//	tuple = dict_find(received, STATUS_KEY);
//	if(tuple) {
//		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
//	}
//	
//	tuple = dict_find(received, MESSAGE_KEY);
//	if(tuple) {
//		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
//	}
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
    s_current_failed_callback();
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    s_current_failed_callback();
}

void message_init() {
    // Register AppMessage handlers
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void message_deinit() {
	app_message_deregister_callbacks();
}

void send_message(MESSAGE_TYPE type,
                  DictionaryIterator *parameters,
                  MessageSucceededCallback succeeded_callback,
                  MessageFailedCallback failed_callback) {
    s_current_succeeded_callback = succeeded_callback;
    s_current_failed_callback = failed_callback;
    
    DictionaryIterator *iter;
    
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_TYPE, type);
    
    dict_write_end(iter);
    app_message_outbox_send();
}