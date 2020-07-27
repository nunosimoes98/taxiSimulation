#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>


#define NUM_TAXI_MESSAGES 10
#define MAX_TEXT 50

#define MSG_TYPE_REGISTER_TAXI          1
#define MSG_TYPE_UNREGISTER_TAXI        2
#define MSG_TYPE_NEW_POS_TAXI           3
#define MSG_TYPE_PICKUP_PASSENGER       4
#define MSG_TYPE_DELIVERED_PASSENGER    5
#define MSG_TYPE_INTERESTED             6

#define MSG_TYPE_NEW_PASSENGER  7
#define MSG_TYPE_EXIT           8

#define ROAD ' '
#define BLOCK 'x'

#define DIRECTION_UP     0
#define DIRECTION_DOWN   1
#define DIRECTION_LEFT   2
#define DIRECTION_RIGHT  3

#define ROAD_CORNER_LT    0
#define ROAD_TOP          1
#define ROAD_CORNER_RT    2
#define ROAD_RIGHT        3
#define ROAD_CORNER_RB    4
#define ROAD_BOTTOM       5
#define ROAD_CORNER_LB    6
#define ROAD_LEFT         7
#define ROAD_CROSS        8
#define ROAD_HORIZONTAL   9
#define ROAD_VERTICAL    10

#define TAXI_STATE_FREE              1
#define TAXI_STATE_BUSY              2
#define TAXI_STATE_PICKING_PASSENGER 3

#define PASSENGER_STATE_WAITING            1
#define PASSENGER_STATE_WAITING_FOR_TAXI   2
#define PASSENGER_STATE_WAITING_IN_TAXI    3
#define NO_TAXI                           -1

#define BUFFER_SIZE  256

#define SEM_TAXI_MESSAGES_READ L"SEM TAXI MESSAGES READ"
#define SEM_TAXI_MESSAGES_WRITE L"SEM TAXI MESSAGES WRITE"
#define SEM_TAXI_MESSAGES_WRITE_POS L"SEM TAXI MESSAGES WRITE POS"
#define SEM_TAXI_NEW_PASSENGER L"SEM TAXI NEW PASSENGER %d"
#define SHM_TAXI_MESSAGES L"SHM TAXI MESSAGES"

#define SEM_TAXI_RESPONSE L"SEM TAXI RESPONSE %d"
#define SHM_TAXI_RESPONSE L"SHM TAXI RESPONSE %d"

#define SHM_MAP_SIZE L"SHM MAP SIZE"
#define SHM_MAP L"SHM MAP"

#define SHM_TAXI_LIST L"SHM TAXI LIST"
#define SHM_PASSENGERS_LIST L" SHM PASSENGERS LIST"
#define SHM_VARIABLES L"SHM VARIABLES"

#define EVENT_MAP_UPDATE L"EVENT UPDATE MAP"

#define NAME_PIPE_TAXI L"\\\\.\\pipe\\PIPE_TAXI_%d"

typedef struct {
	int type;
	int x;
	int y;
	int direction;
	int id;
	TCHAR passenger_name[MAX_TEXT];
} message_t;

typedef struct {
	message_t list[NUM_TAXI_MESSAGES];
	int read_pos;
	int write_pos;
} taxi_messages_shm_t;


typedef struct {
	int x;
	int y;
} point_t;

typedef struct {
	HANDLE sem_can_write;
	HANDLE sem_can_read;
	HANDLE sem_pos_write;
	HANDLE shm;
	// Below variables are all in the same shard memory block
	taxi_messages_shm_t* messages;

} taxi_messages_t;

typedef struct {
	HANDLE sem;
	HANDLE sem_new_passenger;
	HANDLE shm;
	int* response;
	TCHAR *passenger_name;
	point_t* passenger_pos;
} taxi_response_t;




#define ROAD ' '
#define BLOCK 'x'

typedef struct {
	point_t node;
	char type;  // If it is a road or block
} node_t;

typedef struct {
	int* width;
	int* height;
	node_t* nodes;
	HANDLE shm_size;
	HANDLE shm_nodes;
} map_t;