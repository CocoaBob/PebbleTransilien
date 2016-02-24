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
#ifdef PBL_ROUND
    ui_common_init();
#endif
    status_bar_init();
    settings_init();
    locale_init();
    stations_init();
    favorites_init();
    message_init();
    
    ui_push_window(new_window_main_menu());
}

void handle_deinit(void) {
#ifdef PBL_ROUND
    ui_common_deinit();
#endif
    status_bar_deinit(); // Always have 40B memory leak due to tick_timer_service_subscribe()
    favorites_deinit();
    stations_deinit();
    locale_deinit();
    settings_deinit();
    message_deinit(); // Always have 16B memory leak due to app_message_set_context()
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}