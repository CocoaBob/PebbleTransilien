//
//  ui_setup.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

// MARK: Status bar layer

void ui_setup_status_bar(Layer *window_layer, Layer *sibling_layer);

// MARK: Theme

#ifdef PBL_COLOR
void ui_setup_theme(Window *window_layer, MenuLayer *menu_layer);
#elif IS_BW_AND_SDK_2
void ui_setup_theme(Window *window_layer, InverterLayer *inverter_layer);
#endif