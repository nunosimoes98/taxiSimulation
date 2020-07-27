#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>

#include "../ConTaxiIPC/data.h"

#define MAX_SPEED        3
#define MANUAL_PICKUP    1
#define AUTO_PICKUP      2
#define INITIAL_RANGE   10
#define INITIAL_SPEED    1



typedef struct {
	point_t pos;            // Taxi position
	point_t passenger_pos;  // Position where the passenger is
	point_t destiny;       // Where it should drop the passenger
	int has_passenger;
	int id;
	float speed;
	int direction;


	int pickup_mode;
	int pickup_distance;
} taxi_t;

typedef struct {
	/* Used in the SHM for circular buffer */
	taxi_response_t* tr;
	taxi_messages_t* tm;

	taxi_t taxi;
	map_t* map;

	HANDLE thread_taxi_move;
	HANDLE thread_new_passenger;
	HANDLE thread_wait_for_passengers;

	HANDLE pipe;
	HANDLE event;
	OVERLAPPED ovl;

} settings_t;