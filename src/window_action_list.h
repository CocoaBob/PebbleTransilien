//
//  window_action_list.h
//  PebbleTransilien
//
//  Created by CocoaBob on 30/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

typedef char*   (*ActionListGetTitleCallback)(size_t index);
typedef bool    (*ActionListIsEnabledCallback)(size_t index);
typedef void    (*ActionListSelectCallback)(Window *action_list_window, size_t index);

typedef struct {
    size_t num_rows;
    size_t default_selection;
#ifdef PBL_COLOR
    struct {
        GColor background; //!< the color of the left column of the ActionMenu
        GColor foreground; //!< the color of the individual "crumbs" that indicate menu depth
        GColor text;
        GColor text_selected;
        GColor text_disabled;
    } colors;
#endif
    struct ActionListCallbacks {
        ActionListGetTitleCallback get_title;
        ActionListIsEnabledCallback is_enabled;
        ActionListSelectCallback select_click;
    } callbacks;
} ActionListConfig;

void action_list_open(ActionListConfig *config);