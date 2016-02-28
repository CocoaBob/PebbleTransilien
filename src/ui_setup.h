//
//  ui_setup.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

// MARK: Status bar layer

void ui_setup_status_bars(Layer *window_layer, Layer *sibling_layer);

// MARK: Theme

void ui_setup_theme(MenuLayer *menu_layer);

// MARK: Round bottom
#ifdef PBL_ROUND
// Get round bottom bar layer
Layer *round_bottom_bar(GRect frame);
// Update status bar drawing
void round_bottom_bar_update();
// Setup the shared bottom bar
void round_bottom_bar_init();
// Free the memory of the shared bottom bar
void round_bottom_bar_deinit();
#endif