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
    char number[7];         // Train number, null-terminated string, e.g. "133871"
    char code[5];           // Train mission code, null-terminated string, e.g. "POBI"
    char hour[7];           // Train hour, null-terminated string, e.g. "-00:42"
    char platform[3];       // Train dock or Train lane, null-terminated string, e.g. "C"
    size_t terminus;        // Train terminus station index, e.g. 354 (PSL: Paris Saint-Lazare)
} DataModelNextTrain;

typedef struct DataModelTrainDetail {
    char time[7];           // time, the time of arriving
    char platform[3];       // lane, the platform
    size_t station;         // codeGare, Station index, e.g. 354 (PSL: Paris Saint-Lazare)
} DataModelTrainDetail;