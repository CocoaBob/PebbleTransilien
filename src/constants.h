//
//  constants.h
//  PebbleTransilien
//
//  Created by CocoaBob on 09/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#define NULL_FREE(x) {if (x != NULL) free(x); x = NULL;}

#define CELL_MARGIN 4
#define CELL_MARGIN_2 8
#define CELL_MARGIN_3 12
#define CELL_MARGIN_4 16
#define CELL_HEIGHT 44
#define CELL_HEIGHT_2 22
#define CELL_ICON_SIZE 19
#define CELL_SUB_ICON_SIZE 15

#define TEXT_Y_OFFSET -2

#define HEADER_HEIGHT 15

#define FROM_TO_ICON_WIDTH 8
#define FROM_TO_ICON_HEIGHT 26

#define TIME_STRING_LENGTH 7