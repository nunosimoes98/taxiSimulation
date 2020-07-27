#pragma once

#include "..\ConTaxiIPC\data.h"

#include <tchar.h>



// Names are inverted from ConPass project
#define PIPE_NAME_WRITE   L"\\\\.\\pipe\\ConPassRead"
#define PIPE_NAME_READ    L"\\\\.\\pipe\\ConPassWrite"



typedef struct {
	point_t *pos;
	int* id;
	int* direction;
	HANDLE sem; // Semaphore for sending small messags for the taxi
	HANDLE sem_new_passenger; // Semaphore for sending small messags for the taxi
	HANDLE shm;
	int* message; // To send aknowledge to the taxi
	point_t* passenger_pos;
	TCHAR* passenger_name;
	int* state;
	HANDLE pipe_write; // To send stuff to Taxi
} taxi_t;


typedef struct {
	point_t pos;
	TCHAR id[MAX_TEXT];
	point_t destiny;
	char state;
	int taxi_id;
	int number_taxis_interested;
	int* list_taxis_interested;
	HANDLE timer;
} passenger_t;

typedef struct {
	point_t pos;
	int id;
	int direction;
	int state;
} taxi_shm_t;


typedef struct {
	int *max_taxis;
	int *num_taxis;
	taxi_t* list_taxis;
	int allow_taxis_registration;
	HANDLE taxi_shm;
	taxi_shm_t *list_taxis_shm;
	
	taxi_messages_t* taxi_messages;
	HANDLE thread_wait_messages;

	int *max_passengers;
	int *num_passengers;
	passenger_t* list_passengers;
	int wait_for_taxis;
	HANDLE passenger_shm;
	HANDLE variables_shm;

	TCHAR* map_filename;
	map_t map;
	HANDLE event_update_map;

	HANDLE thread_wait_passengers;
	HANDLE pipe_read_passengers;
	HANDLE pipe_write_passengers;
	HANDLE event_read;
	HANDLE event_write;
} settings_t;


typedef struct {
	settings_t* settings;
	passenger_t* passenger;
} timer_arg_t;