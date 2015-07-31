//
//  main.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

void handle_init(void) {
//    persist_delete(102);
//    persist_delete(103);    
    locale_init();
    status_init();
    stations_init();
    message_init();
    push_main_menu_window(true);
}

void handle_deinit(void) {
    status_deinit();
    stations_deinit();
    message_deinit();
    locale_deinit();
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}