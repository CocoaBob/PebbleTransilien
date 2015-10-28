//
//  constants.h
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

//#define ENABLE_TEXT_SCROLL_FOR_APLITE
#define TEXT_SCROLL_IS_ENABLED (!defined(PBL_PLATFORM_APLITE) || defined(ENABLE_TEXT_SCROLL_FOR_APLITE))

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef STATUS_BAR_LAYER_HEIGHT
#define STATUS_BAR_LAYER_HEIGHT 16
#endif

#define STATION_DATA_VERSION 1

#define SETTING_KEY_STATION_DATA_VERSION 0
#define SETTING_KEY_THEME 100
#define SETTING_KEY_LOCALE 101
//#define SETTING_KEY_IS_FAV_ON_LAUNCH 102 // Used in version 1.0
#define SETTING_KEY_FAVORITES 200
#define SETTING_KEY_FAVORITES_COUNT 201

#define NULL_FREE(x) {if (x != NULL) free(x); x = NULL;}

#define CELL_MARGIN 4
#define CELL_MARGIN_2 8
#define CELL_MARGIN_3 12
#define CELL_MARGIN_4 16
#define CELL_HEIGHT 44
#define CELL_HEIGHT_2 22
#define CELL_ICON_SIZE 19
#define CELL_SUB_ICON_SIZE 13

#define TEXT_Y_OFFSET -2

#define HEADER_HEIGHT 15

#define FROM_TO_ICON_WIDTH 8
#define FROM_TO_ICON_HEIGHT 26

#define TIME_STRING_LENGTH 8