# Transilien
An unofficial Pebble Time app of SNCF Transilien

# Database specifications
## Where is it from?
Train station codes and coordinates are extracted from the official app's database. If it's the iOS version, the database is located in `/Documents/Transilien.sqlite`.

## How is it organized?
To facilitate the queries, data is organized into several binaries. They are:

* station\_code.bin (ordered station codes)
* station\_name.bin (ordered station names)
* station\_latlng.bin (station indexes grouped into latitude/longitude grids)

## Data structures
### station\_code.bin
```
[DATA]
	[3 Byte] char[3]			station_code
```
### station\_name.bin
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
First you know the station's index, then read 3 bytes at the position `3*index` in `station_code.bin`.

### Get or Search a station's name
1. Create a char** array based on the data of `station_name.bin`
2. Each char* points to a NULL-terminated string
3. If you know the station's index, then get the pointer of the corresponding NULL-terminated string
4. If you want to search a name, compare all the chars to find the ones you want.

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
		// If exists, get station indexes
	}
}
```

