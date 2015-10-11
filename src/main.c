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
    if (status_is_fav_on_launch() && fav_get_count() > 0) {
        Favorite favorite = fav_at_index(0);
        push_next_trains_window(favorite, true);
    }
}

void handle_init(void) {
//    persist_delete(102);
//    persist_delete(103);
    locale_init();
    status_init();
    stations_init();
    load_favorites();
    
    push_main_menu_window(false);
    
    message_init((MessageCallbacks){
        .message_succeeded_callback = pebble_js_is_ready_callback
    });
}

void handle_deinit(void) {
    unload_favorites();
    status_deinit();
    stations_deinit();
    locale_deinit();
    message_deinit();
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}