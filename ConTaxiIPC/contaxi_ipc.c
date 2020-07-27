// CONTAXI_IPC.cpp : Defines the exported functions for the DLL.
//

#include "contaxi_ipc.h"
#include "../Common/common.h"

#include <stdlib.h>
#include <tchar.h>
#include <string.h>


function_log_ipc_t log_ipc;
function_register_ipc_t register_ipc;
HINSTANCE h_ipc_dll;

CONTAXIIPC_API int load_log_ipc_dll()
{
	h_ipc_dll = LoadLibrary(L"SO2_TP_DLL_32.dll");
	if (h_ipc_dll == NULL) {
		return TAXI_ERROR;
	}

	register_ipc = (function_register_ipc_t)GetProcAddress(h_ipc_dll, "dll_register");
	if (register_ipc == NULL) {
		return TAXI_ERROR;
	}

	log_ipc = (function_log_ipc_t)GetProcAddress(h_ipc_dll, "dll_log");
	if (log_ipc == NULL) {
		return TAXI_ERROR;
	}

	return TAXI_NOERROR;
}

CONTAXIIPC_API void free_log_ipc_dll()
{
	FreeLibrary(h_ipc_dll);
}


CONTAXIIPC_API node_t* get_node(map_t* map, int x, int y)
{
	if (x < 0 || y < 0 || x >* map->width || y >* map->height) {
		return NULL;
	}
	return &map->nodes[x + *map->width * y];
}

CONTAXIIPC_API point_t null_point()
{
	point_t p = { -1, -1 };
	return p;
}

CONTAXIIPC_API int is_null_point(point_t* p)
{
	return (p->x == -1 && p->y == -1);
}


CONTAXIIPC_API void write_taxi_message(taxi_messages_t* tm, message_t* msg)
{
	WaitForSingleObject(tm->sem_can_write, INFINITE);

	WaitForSingleObject(tm->sem_pos_write, INFINITE);

	int pos = tm->messages->write_pos;
	tm->messages->write_pos = (tm->messages->write_pos + 1) % NUM_TAXI_MESSAGES;

	ReleaseSemaphore(tm->sem_pos_write, 1, NULL);

	// Copy message
	tm->messages->list[pos].id = msg->id;
	tm->messages->list[pos].type = msg->type;
	tm->messages->list[pos].x = msg->x;
	tm->messages->list[pos].y = msg->y;
	tm->messages->list[pos].direction = msg->direction;
	_tcscpy_s(tm->messages->list[pos].passenger_name, MAX_TEXT, msg->passenger_name);

	ReleaseSemaphore(tm->sem_can_read, 1, NULL);
}

CONTAXIIPC_API int wait_taxi_response(taxi_response_t* tr)
{
	WaitForSingleObject(tr->sem, INFINITE);

	return *tr->response;
}

CONTAXIIPC_API void wait_taxi_new_passenger(taxi_response_t* tr)
{
	WaitForSingleObject(tr->sem_new_passenger, INFINITE);

}

/*
	Reserve memsory for messages buffer and new passenger_info
*/
CONTAXIIPC_API taxi_messages_t* create_ipc_taxi_messages()
{
	taxi_messages_t* tm = (taxi_messages_t*)malloc(sizeof(taxi_messages_t));
	if (tm == NULL) {
		return NULL;
	}

	// Create semaphores
	tm->sem_can_write = CreateSemaphore(NULL, NUM_TAXI_MESSAGES, NUM_TAXI_MESSAGES, SEM_TAXI_MESSAGES_WRITE);
	tm->sem_can_read = CreateSemaphore(NULL, 0, NUM_TAXI_MESSAGES, SEM_TAXI_MESSAGES_READ);
	tm->sem_pos_write = CreateSemaphore(NULL, 1, 1, SEM_TAXI_MESSAGES_WRITE_POS);
	if (tm->sem_can_write == NULL || tm->sem_can_read == NULL || tm->sem_pos_write == NULL ) {
		free(tm);
		return NULL;
	}
	register_ipc(SEM_TAXI_MESSAGES_WRITE, IPC_TYPE_SEM);
	register_ipc(SEM_TAXI_MESSAGES_READ, IPC_TYPE_SEM);
	register_ipc(SEM_TAXI_MESSAGES_WRITE_POS, IPC_TYPE_SEM);

	// Create shared memory handle for 3 variables
	// - messages buffer
	// - passenger position 
	// - passenger name
	size_t size = sizeof(taxi_messages_shm_t);

	tm->shm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
		size, SHM_TAXI_MESSAGES);
	if (tm->shm == NULL) {
		CloseHandle(tm->sem_can_write);
		CloseHandle(tm->sem_can_read);
		CloseHandle(tm->sem_pos_write);
		free(tm);
		return NULL;
	}
	register_ipc(SHM_TAXI_MESSAGES, IPC_TYPE_FILE_MAPPING);

	// Allocate shared memory
	tm->messages = (taxi_messages_shm_t*)MapViewOfFile(tm->shm,
		FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size);
	if (tm->messages == NULL) {
		CloseHandle(tm->sem_can_write);
		CloseHandle(tm->sem_can_read);
		CloseHandle(tm->sem_pos_write);
		CloseHandle(tm->shm);
		free(tm);
		return NULL;
	}
	
	register_ipc(SHM_TAXI_MESSAGES, IPC_TYPE_VIEW_FILE_MAPPING);

	return tm;
}



CONTAXIIPC_API void free_ipc_taxi_messages(taxi_messages_t* tm)
{
	CloseHandle(tm->sem_pos_write);
	CloseHandle(tm->sem_can_read);
	CloseHandle(tm->sem_can_write);
	UnmapViewOfFile(tm->messages);
	CloseHandle(tm->shm);
	
	free(tm);
}


CONTAXIIPC_API HANDLE create_taxi_response_sem(int taxi_id)
{
	TCHAR name[50];
	_stprintf_p(name, 50, SEM_TAXI_RESPONSE, taxi_id);
	register_ipc(name, IPC_TYPE_SEM);
	return CreateSemaphore(NULL, 0, 1, name);
}


CONTAXIIPC_API HANDLE create_taxi_response_shm(int taxi_id)
{
	TCHAR name[50];
	_stprintf_p(name, 50, SHM_TAXI_RESPONSE, taxi_id);
	register_ipc(name, IPC_TYPE_FILE_MAPPING);
	return CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) + sizeof(point_t) + sizeof(TCHAR) * MAX_TEXT, name);
}

CONTAXIIPC_API HANDLE create_taxi_named_pipe(int taxi_id, HANDLE* event, OVERLAPPED *ovl)
{
	TCHAR name[50];
	_stprintf_p(name, 50, NAME_PIPE_TAXI, taxi_id);

	*event = CreateEvent(NULL, FALSE, FALSE, NULL);
	ovl->hEvent = *event;

	HANDLE handle = CreateNamedPipe(name,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_WAIT | PIPE_TYPE_MESSAGE
		| PIPE_READMODE_MESSAGE, 1, sizeof(TCHAR) * BUFFER_SIZE, sizeof(TCHAR) * BUFFER_SIZE, 1000, NULL);

	ConnectNamedPipe(handle, ovl);
	WaitForSingleObject(*event, INFINITE);

	return handle;
}

CONTAXIIPC_API HANDLE open_taxi_named_pipe(int taxi_id)
{
	TCHAR name[50];
	_stprintf_p(name, 50, NAME_PIPE_TAXI, taxi_id);


	if (!WaitNamedPipe(name, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(L"Error connecting to read pipe.\n");
		return INVALID_HANDLE_VALUE;
	}
	return CreateFile(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}



CONTAXIIPC_API HANDLE create_taxi_sem_new_passenger(int taxi_id)
{
	TCHAR name[50];
	_stprintf_p(name, 50, SEM_TAXI_NEW_PASSENGER, taxi_id);

	register_ipc(name, IPC_TYPE_SEM);
	return CreateSemaphore(NULL, 0, 1, name);
}


CONTAXIIPC_API taxi_response_t* create_ipc_taxi_response(int taxi_id)
{
	taxi_response_t* tr = (taxi_response_t*)malloc(sizeof(taxi_response_t));
	if (tr == NULL) {
		return NULL;
	}

	// Create semaphores
	tr->sem = create_taxi_response_sem(taxi_id);
	if (tr->sem == NULL) {
		free(tr);
		return NULL;
	}

	tr->sem_new_passenger = create_taxi_sem_new_passenger(taxi_id);
	if (tr->sem == NULL) {
		CloseHandle(tr->sem);
		free(tr);
		return NULL;
	}


	// Create shared memory handle
	tr->shm = create_taxi_response_shm(taxi_id);
	if (tr->shm == NULL) {
		CloseHandle(tr->sem);
		CloseHandle(tr->sem_new_passenger);
		free(tr);
		return NULL;
	}

	// Map shared memory
	tr->response = (int*)MapViewOfFile(tr->shm,
		FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(int) + sizeof(point_t) + sizeof(TCHAR) * MAX_TEXT);
	if (tr->response == NULL) {
		CloseHandle(tr->sem);
		CloseHandle(tr->sem_new_passenger);
		CloseHandle(tr->shm);
		free(tr);
		return NULL;
	}

	tr->passenger_pos = (point_t*)(tr->response + sizeof(int));
	tr->passenger_name = (TCHAR*)(tr->passenger_pos + sizeof(point_t));

	return tr;
}

CONTAXIIPC_API void free_ipc_taxi_response(taxi_response_t* tr)
{
	CloseHandle(tr->sem);
	CloseHandle(tr->shm);
	UnmapViewOfFile(tr->response);
	free(tr);
}



CONTAXIIPC_API map_t* init_ipc_map()
{
	map_t* map = (map_t*)malloc(sizeof(map_t));
	if (map == NULL) {
		return NULL;
	}

	// create shared memory
	map->shm_size = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHM_MAP_SIZE);
	if (map->shm_size == NULL) {

		//_tprintf(L"It was not possible to create windows objects.\n Error: %s\n", get_last_error());
		return NULL;
	}
	register_ipc(SHM_MAP_SIZE, IPC_TYPE_FILE_MAPPING);
	map->height = (int*)MapViewOfFile(map->shm_size, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(int) * 2);
	if (map->height == NULL) {
		//_tprintf(TEXT("It was not possible to create shared memory map \n Error:%s\n"), GetLastError());
		CloseHandle(map->shm_size);
		free(map);
		return NULL;
	}
	map->width = &map->height[1];

	int nodes_size = (*map->height) * (*map->width) * sizeof(node_t);

	map->shm_nodes = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHM_MAP);
	if (map->shm_nodes == NULL) {
		CloseHandle(map->shm_size);
		UnmapViewOfFile(map->height);
		free(map);
		return NULL;
	}
	register_ipc(SHM_MAP, IPC_TYPE_FILE_MAPPING);

	map->nodes = (node_t*)MapViewOfFile(map->shm_nodes, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, nodes_size);
	if (map->nodes == NULL) {
		CloseHandle(map->shm_size);
		CloseHandle(map->shm_nodes);
		UnmapViewOfFile(map->height);
		free(map);
		return NULL;
	}

	return map;
}

CONTAXIIPC_API void free_ipc_map(map_t* map)
{
	CloseHandle(map->shm_nodes);
	CloseHandle(map->shm_size);
	UnmapViewOfFile(map->height);
	UnmapViewOfFile(map->nodes);
	free(map);
}
