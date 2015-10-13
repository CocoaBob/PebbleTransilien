[<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/banner.png" width="720"/>](https://apps.getpebble.com/en_US/application/561250ba1cdcfc612600008c)

# Transilien
PebbleTransilien is an unofficial app for checking SNCF Transilien™ trains on your wrist. Transilien™ is the brand name of the suburban railway service of the SNCF-owned railway network operating within the Île-de-France.

<p align="center">
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/0.gif" width="144" height="168"/>
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/1.gif" width="144" height="168"/>
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/2.gif" width="144" height="168"/>
</p>

<p align="center">
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/3.gif" width="144" height="168"/>
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/4.gif" width="144" height="168"/>
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/5.png" width="144" height="168"/>
</p>

<p align="center">
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/6.png" width="144" height="168"/>
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/7.png" width="144" height="168"/>
<img src="https://raw.githubusercontent.com/CocoaBob/PebbleTransilien/master/screenshots/8.png" width="144" height="168"/>
</p>

# Database specifications
## Where is the data from?
Train station codes and coordinates are extracted from the official app's database. If it's the iOS version, the database is located in `/Documents/Transilien.sqlite`.

It's a SQLite database, we can simply query what we want, for example, I need stations' codes, names, coordinates (average value if duplicated), so my query is like the following one:

```
SELECT ZGARE.ZCODETR3A AS CODE, ZGARE.ZNAME AS NAME,AVG(ZPOSITION.ZLATITUDE) AS LAT,AVG(ZPOSITION.ZLONGITUDE) AS LONG
FROM ZGARE, ZPOSITION
WHERE ZGARE.Z_PK = ZPOSITION.ZGARE
GROUP BY CODE
ORDER BY CODE
```

## How is it organized?
To facilitate the usages, data is organized into several binaries by using this Mac tool [TransilienStations](https://github.com/CocoaBob/TransilienStations). (Remember to pull CocoaPods dependencies before building in Xcode.)

The binaries are:

* station\_code.bin (ordered station codes)
* station\_name\_pos.bin (positions of all the names in `station_name.bin`)
* station\_name.bin (all station names in alphabetic order)
* station\_name\_search\_pos.bin (positions of all the names in `station_name_search.bin`)
* station\_name\_search.bin (all station names without white space or accent in alphabetic order)
* station\_latlng.bin (station indexes grouped into latitude/longitude grids)

For Windows/Linux users, don't worry, let me explain the data structures, so that you can develop another tool to generate the same binaries.

## Data structures
### station\_code.bin
```
[DATA]
	[3 Byte] char[3]			station_code
```
### station\_name\_pos.bin
```
[DATA]
	[2 Byte] uint8_t[2]			station_name_position
```
### station\_name.bin
```
[DATA]
	[N Byte] char*				station_name (NULL-terminated string)
```
### station\_name\_search\_pos.bin
```
[DATA]
	[2 Byte] uint8_t[2]			station_name_position
```
### station\_name\_search.bin
```
[DATA]
	[N Byte] char*				station_name (NULL-terminated string)
```
### station\_latlng.bin

```
[HEAD]
	[4 Byte] float				min_lat
	[4 Byte] float				max_lat
	[4 Byte] float				min_lng
	[4 Byte] float				max_lng
	[2 Byte] unsigned int		grid_count_lat
	[2 Byte] unsigned int		grid_count_lng
[DATA]
	[2 Byte] unsigned int		latitude_longitude_indexes_key
	[2 Byte] unsigned int		number_of_stations
	[2xN Byte] unsigned int		station_indexes
```
## Usage
### Get a station's code
1. First you have to know the station's `index`
2. Then read 3 bytes at the position `3*index` of `station_code.bin`.

### Get or Search a station's name
* If you know a station's `index`
	1. Read 2 bytes at position `2*index` of `station_name_pos.bin` and store them into a variable `uint8_t pos[2]`
	2. Then we get the name position `size_t position = (pos[0] << 8) + pos[1]`
	3. Then at the `position` of `station_name.bin`, get name's length by using `strlen()` (names are null-terminated strings), then read a string in that length. 

* If you want to search a name
	1. Compare all the letters in `station_name_search.bin` to find the index you want.
	2. To reduce memory footprint, you can compare names one by one with the help of `station_name_search_pos.bin`, which contains each name's position.

### Search nearby stations
1. First you have the current location `curr_lat` and `curr_lng`.
2. Calculate the grid sizes, e.g. `grid_size_lat` = `(max_lat - min_lat) / count_lat`
3. Calculate the indexes, e.g. `floorf((curr_lat - min_lat)/grid_size_lat)`
4. The key is `index_lat * 100 + index_lng`
5. Create a dictionary with the keys and station indexes
6. Get all the stations in the 9 grids around the position

```
for (int offset_lat = -1; offset_lat <= 1; ++ offset_lat) {
	for (int offset_lng = -1; offset_lng <= 1; ++ offset_lng) {
		int32_t key = (index_lat + offset_lat)* 100 + (index_lng + offset_lng);
		// Check if the key exists
		// If true, get stations' indexes
	}
}
```
## RESTful web APIs
To request the realtime information from SNCF, we have to use their web services. I'm lazy, I didn't request for the [Open Data](https://data.sncf.com/) APIs, so I just use the same APIs as the official iOS version of [SNCF Transilien](https://itunes.apple.com/fr/app/sncf-transilien/id451963011?mt=8).

```
http://transilien.ods.ocito.com/ods/transilien/iphone
```

This is the request URL used in the current iOS version of SNCF Translien.

All the requests are HTTP POST requests with POST Body. All the request/response data are in JSON format (`"Content-Type": "application/json; charset=utf-8"`).

### Request nearby stations

POST Body

```
[
  {
    "serial": "1",
    "target": "/transilien/autourDeMoi",
    "map": {
      "longitude": 2.265482,
      "latitude": 48.905355,
      "distance": 300,
      "hasBeenFiltered": "true"
    }
  }
]
```

Response

```
[
  {
    "binary": null,
    "data": [
      {
        "codeDUA": "DUA59:4274261",
        "codeTR3A": null,
        "latitude": 48.905146072906966,
        "lignes": [
          {
            "directions": [
              "Pont de Levallois / Bécon",
              "AUDRA"
            ],
            "externalCode": "DUA100100167",
            "nom": "167",
            "type": "Bus"
          }
        ],
        "longitude": 2.263350926842547,
        "modeTypeExternalCode": "Bus",
        "nom": "MICHEL RICARD",
        "stopAreaExternalCode": "DUA59:4274261",
        "type": null,
        "x": 594634,
        "y": 2434100
      },
      {
        "codeDUA": "DUA59:4037345",
        "codeTR3A": null,
        "latitude": 48.90698093809426,
        "lignes": [
          {
            "directions": [
              "MADELEINE",
              "ARGENTEUIL"
            ],
            "externalCode": "DUA100987752",
            "nom": "N52",
            "type": "Bus"
          }
        ],
        "longitude": 2.2658159459896807,
        "modeTypeExternalCode": "Bus",
        "nom": "CHEVREUL",
        "stopAreaExternalCode": "DUA59:4037345",
        "type": null,
        "x": 594815,
        "y": 2434304
      },
      {
        "codeDUA": "DUA76:445",
        "codeTR3A": "BEC",
        "latitude": 48.9055805743721,
        "lignes": [
          {
            "directions": [
              "VERSAILLES RIVE DROITE",
              "PARIS SAINT-LAZARE"
            ],
            "externalCode": "DUA800854542",
            "nom": "L",
            "type": "Bus"
          }
        ],
        "longitude": 2.268571738926075,
        "modeTypeExternalCode": "Bus",
        "nom": "GARE DE BECON LES BRUYERES",
        "stopAreaExternalCode": "DUA8738200",
        "type": null,
        "x": 595017,
        "y": 2434148
      },
      {
        "codeDUA": "DUA76:446",
        "codeTR3A": "BEC",
        "latitude": 48.9055805743721,
        "lignes": [
          {
            "directions": [
              "VERSAILLES RIVE DROITE",
              "PARIS SAINT-LAZARE"
            ],
            "externalCode": "DUA800854542",
            "nom": "L",
            "type": "Bus"
          }
        ],
        "longitude": 2.268571738926075,
        "modeTypeExternalCode": "Bus",
        "nom": "GARE DE BECON LES BRUYERES",
        "stopAreaExternalCode": "DUA8738200",
        "type": null,
        "x": 595017,
        "y": 2434148
      }
    ]
  }
]
```
### Request next trains for one station

POST Body

```
[
  {
    "target": "/transilien/getNextTrains",
    "map": {
      "codeArrivee": "",
      "codeDepart": "BEC",
      "theoric": "false"
    }
  }
]
```
Response

```
[
  {
    "binary": null,
    "data": [
      {
        "trainDock": "D",
        "trainHour": "26/07/2015 00:19",
        "trainLane": null,
        "trainMention": "Retardé",
        "trainMissionCode": "SEBO",
        "trainNumber": "134735",
        "trainTerminus": "SNB",
        "type": "R"
      },
      {
        "trainDock": "C",
        "trainHour": "26/07/2015 00:26",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "PORO",
        "trainNumber": "134708",
        "trainTerminus": "PSL",
        "type": "R"
      },
      {
        "trainDock": "B",
        "trainHour": "26/07/2015 00:28",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "SOPE",
        "trainNumber": "135493",
        "trainTerminus": "SVL",
        "type": "R"
      },
      {
        "trainDock": "D",
        "trainHour": "26/07/2015 00:31",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "VULE",
        "trainNumber": "133877",
        "trainTerminus": "VRD",
        "type": "R"
      },
      {
        "trainDock": "A",
        "trainHour": "26/07/2015 00:33",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "POPI",
        "trainNumber": "135354",
        "trainTerminus": "PSL",
        "type": "R"
      }
    ],
    "headers": null,
    "list": [
      "La version d'application utilisée est obsolète. Nous vous invitons à télécharger la dernière version en date afin de profiter de toutes les fonctionnalités à jour."
    ],
    "listOfMap": null,
    "map": {
      "codeArrivee": "",
      "hasBeenFiltered": "true",
      "codeDepart": "BEC"
    },
    "serial": 0,
    "state": 200,
    "target": null
  }
]
```
###Request next trains from one station to another station

POST Body

```
[
  {
    "serial": 0,
    "target": "/transilien/getNextTrains",
    "map": {
      "codeArrivee": "PSL",
      "codeDepart": "BEC",
      "theoric": "false"
    }
  },
  {
    "serial": 1,
    "target": "/transilien/getNextTrains",
    "map": {
      "codeArrivee": "BEC",
      "codeDepart": "PSL",
      "theoric": "false"
    }
  }
]
```
Response

```

[
  {
    "binary": null,
    "data": [
      {
        "trainDock": "C",
        "trainHour": "26/07/2015 00:26",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "PORO",
        "trainNumber": "134708",
        "trainTerminus": "PSL",
        "type": "R"
      },
      {
        "trainDock": "A",
        "trainHour": "26/07/2015 00:33",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "POPI",
        "trainNumber": "135354",
        "trainTerminus": "PSL",
        "type": "R"
      },
      {
        "trainDock": "C",
        "trainHour": "26/07/2015 00:42",
        "trainLane": null,
        "trainMention": null,
        "trainMissionCode": "POBO",
        "trainNumber": "133868",
        "trainTerminus": "PSL",
        "type": "R"
      }
    ],
    "headers": null,
    "list": [
      "La version d'application utilisée est obsolète. Nous vous invitons à télécharger la dernière version en date afin de profiter de toutes les fonctionnalités à jour."
    ],
    "listOfMap": null,
    "map": {
      "codeArrivee": "PSL",
      "hasBeenFiltered": "true",
      "codeDepart": "BEC"
    },
    "serial": 0,
    "state": 200,
    "target": null
  },
  {
    "binary": null,
    "data": [
      {
        "trainDock": null,
        "trainHour": "26/07/2015 00:25",
        "trainLane": "3",
        "trainMention": null,
        "trainMissionCode": "VULE",
        "trainNumber": "133877",
        "trainTerminus": "VRD",
        "type": "R"
      },
      {
        "trainDock": null,
        "trainHour": "26/07/2015 00:43",
        "trainLane": "BL",
        "trainMention": null,
        "trainMissionCode": "SEBO",
        "trainNumber": "134741",
        "trainTerminus": "SNB",
        "type": "R"
      },
      {
        "trainDock": null,
        "trainHour": "26/07/2015 00:49",
        "trainLane": "BL",
        "trainMention": null,
        "trainMissionCode": "SOPE",
        "trainNumber": "135497",
        "trainTerminus": "SVL",
        "type": "R"
      }
    ],
    "headers": null,
    "list": [
      "La version d'application utilisée est obsolète. Nous vous invitons à télécharger la dernière version en date afin de profiter de toutes les fonctionnalités à jour."
    ],
    "listOfMap": null,
    "map": {
      "codeArrivee": "BEC",
      "hasBeenFiltered": "true",
      "codeDepart": "PSL"
    },
    "serial": 1,
    "state": 200,
    "target": null
  }
]
```
### Request details for one train

POST Body

```
[
  {
    "target": "/transilien/getTrainDetails",
    "map": {
      "trainNumber": "133871",
      "theoric": "false"
    }
  }
]
```
Response

```
[
  {
    "binary": null,
    "data": [
      {
        "codeGare": "PSL",
        "lane": "BL",
        "mention": "N",
        "time": "23/07/2015 23:55",
        "typeTrain": "C"
      },
      {
        "codeGare": "BEC",
        "lane": "12",
        "mention": "N",
        "time": "24/07/2015 00:01",
        "typeTrain": "C"
      },
      {
        "codeGare": "KOU",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:04",
        "typeTrain": "C"
      },
      {
        "codeGare": "LDU",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:06",
        "typeTrain": "C"
      },
      {
        "codeGare": "PTX",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:08",
        "typeTrain": "C"
      },
      {
        "codeGare": "MVH",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:11",
        "typeTrain": "C"
      },
      {
        "codeGare": "VDO",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:13",
        "typeTrain": "C"
      },
      {
        "codeGare": "SCD",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:15",
        "typeTrain": "C"
      },
      {
        "codeGare": "VDV",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:18",
        "typeTrain": "C"
      },
      {
        "codeGare": "CWJ",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:21",
        "typeTrain": "C"
      },
      {
        "codeGare": "VFD",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:23",
        "typeTrain": "C"
      },
      {
        "codeGare": "MFL",
        "lane": "1",
        "mention": "N",
        "time": "24/07/2015 00:25",
        "typeTrain": "C"
      },
      {
        "codeGare": "VRD",
        "lane": "2",
        "mention": "M",
        "time": "24/07/2015 00:28",
        "typeTrain": "C"
      }
    ],
    "headers": null,
    "list": null,
    "listOfMap": null,
    "map": {
      "trainNumber": "133871"
    },
    "serial": 0,
    "state": 200,
    "target": null
  }
]
```

### Request details for multiple trains

POST Body

```
[
  {
    "target": "/transilien/getAllTrainsDetails",
    "map": {
      "trainNumbers": "133871,133864",
      "theoric": "false"
    },
    "serial": "18"
  }
]
```
Response

```
[
  {
    "binary": null,
    "data": {
      "133871": [
        {
          "codeGare": "VDV",
          "lane": "1",
          "mention": "N",
          "time": "26/07/2015 00:19",
          "typeTrain": "C"
        },
        {
          "codeGare": "CWJ",
          "lane": "1",
          "mention": "N",
          "time": "26/07/2015 00:22",
          "typeTrain": "C"
        },
        {
          "codeGare": "VFD",
          "lane": "1",
          "mention": "N",
          "time": "26/07/2015 00:24",
          "typeTrain": "C"
        },
        {
          "codeGare": "MFL",
          "lane": "1",
          "mention": "N",
          "time": "26/07/2015 00:26",
          "typeTrain": "C"
        },
        {
          "codeGare": "VRD",
          "lane": "2",
          "mention": "M",
          "time": "26/07/2015 00:29",
          "typeTrain": "C"
        }
      ],
      "133864": [
        {
          "codeGare": "VRD",
          "lane": "2",
          "mention": "",
          "time": "23:45",
          "typeTrain": "C"
        },
        {
          "codeGare": "MFL",
          "lane": "2",
          "mention": "",
          "time": "23:47",
          "typeTrain": "C"
        },
        {
          "codeGare": "VFD",
          "lane": "2",
          "mention": "",
          "time": "23:49",
          "typeTrain": "C"
        },
        {
          "codeGare": "CWJ",
          "lane": "2",
          "mention": "",
          "time": "23:51",
          "typeTrain": "C"
        },
        {
          "codeGare": "VDV",
          "lane": "2",
          "mention": "",
          "time": "23:54",
          "typeTrain": "C"
        },
        {
          "codeGare": "SCD",
          "lane": "2",
          "mention": "",
          "time": "23:57",
          "typeTrain": "C"
        },
        {
          "codeGare": "VDO",
          "lane": "2",
          "mention": "",
          "time": "23:59",
          "typeTrain": "C"
        },
        {
          "codeGare": "MVH",
          "lane": "2",
          "mention": "",
          "time": "00:02",
          "typeTrain": "C"
        },
        {
          "codeGare": "PTX",
          "lane": "2",
          "mention": "",
          "time": "00:04",
          "typeTrain": "C"
        },
        {
          "codeGare": "LDU",
          "lane": "2",
          "mention": "",
          "time": "00:07",
          "typeTrain": "C"
        },
        {
          "codeGare": "KOU",
          "lane": "2",
          "mention": "",
          "time": "00:09",
          "typeTrain": "C"
        },
        {
          "codeGare": "BEC",
          "lane": null,
          "mention": "",
          "time": "00:11",
          "typeTrain": "C"
        },
        {
          "codeGare": "PSL",
          "lane": "BL",
          "mention": "M",
          "time": "26/07/2015 00:19",
          "typeTrain": "C"
        }
      ]
    },
    "headers": null,
    "list": null,
    "listOfMap": null,
    "map": {
      "trainNumbers": "133871,133864"
    },
    "serial": 18,
    "state": 200,
    "target": null
  }
]
```

## The MIT License (MIT)

Copyright (c) 2015 CocoaBob

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Donation

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=E3GKFYXYUZAQ2)