//
//  main.c
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>

#include "main_menu_window.h"

void handle_init(void) {
    push_main_menu_window();
}

void handle_deinit(void) {
    
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}