//
//  ui_status_bar.h
//  PebbleTransilien
//
//  Created by CocoaBob on 21/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

// Show a flickering alert in the status bar
void status_bar_low_memory_alert();

// Get status bar layer
Layer *status_bar(GRect frame);

// Update status bar drawing
void status_bar_update();

// Setup Connection Service
void status_bar_init();
// Free the memory of the shared status bar
void status_bar_deinit();