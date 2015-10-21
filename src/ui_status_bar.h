//
//  ui_status_bar.h
//  PebbleTransilien
//
//  Created by CocoaBob on 21/10/15.
//  Copyright © 2015 CocoaBob. All rights reserved.
//

#pragma once

Layer *status_bar(GRect frame);

void status_bar_update();

// Free the memory of the shared status bar
void status_bar_deinit();