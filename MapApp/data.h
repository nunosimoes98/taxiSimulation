#pragma once

#include "../ConTaxiIPC/data.h"

#define TILE_SIZE   24
#define SINGLE_INSTANCE_MUTEX_NAME L"MapInfoTaxi"

typedef struct {
	point_t pos;
	char id[MAX_TEXT];
	point_t destiny;
	char state;
	int taxi_id;
} passenger_t;

typedef struct {
	point_t pos;
	int id;
	int direction;
	int state;
} taxi_t;


typedef struct {
	HANDLE variables_shm;

	int* max_taxis;
	int* num_taxis;
	taxi_t* list_taxis;
	HANDLE taxi_shm;

	int* max_passengers;
	int* num_passengers;
	passenger_t* list_passengers;
	HANDLE passenger_shm;

	map_t *map;
	HBITMAP img_map_background;
	HBITMAP img_building;
	HBITMAP img_taxi[4];
	HBITMAP img_taxi_free[4];
	HBITMAP img_taxi_busy[4];
	HBITMAP img_passenger;
	HBITMAP img_road[11];

} settings_t;
