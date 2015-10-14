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

typedef struct DataModelNextTrain {
    char *code;                         // Train mission code, null-terminated string, e.g. "POBI"
    time_t hour;                        // Train hour, 8 Bytes unsigned integer, seconds since January 1st 1970, e.g. "1437738167"
    char *platform;                     // Train dock or Train lane, null-terminated string, e.g. "C"
    StationIndex terminus;              // Train terminus station index, 2 Bytes unsigned integer, e.g. 354 (PSL: Paris Saint-Lazare)
    char *number;                       // Train number, e.g. "133871" or "RATP-ZEMA-150803"
    char *mention;                      // Train mention, e.g. "supprimé" or retardé"
} DataModelNextTrain;

typedef struct DataModelTrainDetail {
    time_t time;                        // time, the time of arriving, 8 Bytes unsigned integer, seconds since January 1st 1970, e.g. "1437738167"
    StationIndex station;                     // codeGare, Station index, e.g. 354 (PSL: Paris Saint-Lazare)
} DataModelTrainDetail;