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
    status_bar_init();
    settings_init();
    locale_init();
    stations_init();
    favorites_init();
    message_init();
    
    ui_push_window(new_window_main_menu());
}

void handle_deinit(void) {
    status_bar_deinit();
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