//
//  window_action_list.h
//  PebbleTransilien
//
//  Created by CocoaBob on 30/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

typedef GColor  (*ActionListGetBackgroundColorCallback)(void);
typedef GColor  (*ActionListGetTextColorCallback)(void);
typedef GColor  (*ActionListGetHighlightTextColorCallback)(void);
typedef GColor  (*ActionListGetDisabledTextColorCallback)(void);
typedef GColor  (*ActionListGetBarColorCallback)(void);
typedef size_t  (*ActionListGetNumberOfRowsCallback)(void);
typedef size_t  (*ActionListGetDefaultSelectionCallback)(void);
typedef char*   (*ActionListGetTitleCallback)(size_t index);
typedef bool    (*ActionListIsEnabledCallback)(size_t index);
typedef void    (*ActionListSelectCallback)(Window *action_list_window, size_t index);

typedef struct ActionListCallbacks {
    ActionListGetBackgroundColorCallback get_background_color;
    ActionListGetTextColorCallback get_text_color;
    ActionListGetHighlightTextColorCallback get_highlight_text_color;
    ActionListGetDisabledTextColorCallback get_disabled_text_color;
    ActionListGetBarColorCallback get_bar_color;
    ActionListGetNumberOfRowsCallback get_num_rows;
    ActionListGetDefaultSelectionCallback get_default_selection;
    ActionListGetTitleCallback get_title;
    ActionListIsEnabledCallback is_enabled;
    ActionListSelectCallback select_click;
} ActionListCallbacks;

void action_list_present_with_callbacks(ActionListCallbacks callbacks);