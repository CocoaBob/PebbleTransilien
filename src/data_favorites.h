//
//  data_favorites.h
//  PebbleTransilien
//
//  Created by CocoaBob on 10/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

#include "data_model.h"

#define FAV_MAX_COUNT 64// 4*64=256, 256 is persist data max length

typedef void (*FavoriteRequestCallback)(void *context);
DataModelNextTrainFavorite *fav_get_next_trains(Favorite favorite);
void fav_start_requests(FavoriteRequestCallback callback, void *context);
void fav_stop_requests();

void favorites_init();
void favorites_deinit();
size_t fav_get_count();
Favorite fav_at_index(size_t index);
bool fav_add(StationIndex from, StationIndex to);
bool fav_delete_at_index(size_t index);
bool fav_move_up_index(size_t index);
bool fav_exists(Favorite i_fav);