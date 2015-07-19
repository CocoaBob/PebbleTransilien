//
//  status_bar.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#ifdef PBL_SDK_3
void status_bar_set_colors(StatusBarLayer *status_bar_layer);
void window_add_status_bar(Layer *window_layer, StatusBarLayer **status_bar_layer, Layer **status_bar_overlay_layer);
#endif