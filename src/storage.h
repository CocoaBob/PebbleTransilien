//
//  storage.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

bool storage_get_theme();
void storage_set_theme(bool is_dark);

bool storage_get_locale(char *locale);
void storage_set_locale(const char* lang);

bool storage_get_favorites(void *favorites);
void storage_set_favorites(const void *favorites);
