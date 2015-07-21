//
//  data_model.h
//  PebbleTransilien
//
//  Created by CocoaBob on 21/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

typedef struct DataModelFromTo {
    size_t from;
    size_t to;
} DataModelFromTo;

typedef DataModelFromTo Favorite;

typedef struct DataModelNextTrain {
    char number[6];         // Train number, e.g. 133871
    char mission_code[4];   // Train mission code, e.g. POBI
    char hour[5];           // Train hour, e.g. 00:42
    char platform[2];       // Train dock or Train lane, e.g. C
    size_t terminus;        // Train terminus station index, e.g. 354 (PSL: Paris Saint-Lazare)
} DataModelNextTrain;