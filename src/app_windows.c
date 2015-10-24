//
//  app_windows.c
//  PebbleTransilien
//
//  Created by CocoaBob on 24/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#include <pebble.h>
#include "headers.h"

void window_push(Window *window) {
    if (window != NULL) {
        window_stack_push(window, true);
    }
}
