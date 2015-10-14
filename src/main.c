//
//  main.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

static void pebble_js_is_ready_callback(DictionaryIterator *received) {
    if (settings_is_fav_on_launch() && fav_get_count() > 0) {
        Favorite favorite = fav_at_index(0);
        push_window_next_trains(favorite, true);
    }
}

void handle_init(void) {
//    persist_delete(200);
//    persist_delete(201);
    settings_init();
    locale_init();
    stations_init();
    favorites_init();
    services_init();
    
    push_window_main_menu(false);
    
    message_init((MessageCallbacks){
        .message_succeeded_callback = pebble_js_is_ready_callback
    });
}

void handle_deinit(void) {
    services_deinit();
    favorites_deinit();
    stations_deinit();
    locale_deinit();
    settings_deinit();
    message_deinit();
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}