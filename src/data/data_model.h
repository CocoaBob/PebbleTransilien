//
//  data_model.h
//  PebbleTransilien
//
//  Created by CocoaBob on 21/07/15.
//  Copyright (c) 2015 CocoaBob. All rights reserved.
//

#pragma once

// MARK: DataModel

typedef size_t StationIndex;

typedef struct DataModelFromTo {
    StationIndex from;
    StationIndex to;
} DataModelFromTo;

typedef DataModelFromTo Favorite;

#define TRAIN_NUMBER_LENGTH 24

typedef struct DataModelNextTrain {
    char code[5];                       // Train mission code, null-terminated string, e.g. "POBI"
    time_t hour;                        // Train hour, 8 Bytes unsigned integer, seconds since January 1st 1970, e.g. "1437738167"
    char platform[3];                   // Train dock or Train lane, null-terminated string, e.g. "C"
    StationIndex terminus;              // Train terminus station index, 2 Bytes unsigned integer, e.g. 354 (PSL: Paris Saint-Lazare)
    char number[TRAIN_NUMBER_LENGTH];   // Train number, e.g. "133871" or "RATP-ZEMA-150803"
} DataModelNextTrain;

typedef struct DataModelTrainDetail {
    time_t time;                        // time, the time of arriving, 8 Bytes unsigned integer, seconds since January 1st 1970, e.g. "1437738167"
    StationIndex station;                     // codeGare, Station index, e.g. 354 (PSL: Paris Saint-Lazare)
} DataModelTrainDetail;