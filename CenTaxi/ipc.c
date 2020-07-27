#include <tchar.h>
#include <stdlib.h>
#include <strsafe.h>


#include "ipc.h"
#include "constants.h"
#include "data.h"
#include "../ConTaxiIPC/contaxi_ipc.h"
#include "../Common/common.h"
#include "../Console/Console.h"


int create_taxi_ipc_resources(taxi_t* taxi)
{
	// Create semaphores
	taxi->sem = create_taxi_response_sem(*taxi->id);
	if (taxi->sem == NULL) {
		return TAXI_ERROR;
	}

	taxi->sem_new_passenger = create_taxi_sem_new_passenger(*taxi->id);
	if (taxi->sem_new_passenger == NULL) {
		CloseHandle(taxi->sem);
		return TAXI_ERROR;
	}

	// Create shared memory block
	taxi->shm = create_taxi_response_shm(*taxi->id);
	if (taxi->shm == NULL) {
		CloseHandle(taxi->sem);
		CloseHandle(taxi->sem_new_passenger);
		return TAXI_ERROR;
	}

	taxi->message = (int*)MapViewOfFile(taxi->shm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(int) + sizeof(point_t) + sizeof(TCHAR) * MAX_TEXT);
	if (taxi->message == NULL) {
		CloseHandle(taxi->sem);
		CloseHandle(taxi->sem_new_passenger);
		CloseHandle(taxi->shm);
		return TAXI_ERROR;
	}

	taxi->passenger_pos = (point_t*)(taxi->message + sizeof(int));
	taxi->passenger_name = (TCHAR*)(taxi->passenger_pos + sizeof(point_t));

	return TAXI_NOERROR;
}

void free_taxi_ipc_resources(taxi_t* taxi)
{
	CloseHandle(taxi->sem);
	UnmapViewOfFile(taxi->message);
	CloseHandle(taxi->shm);
	CloseHandle(taxi->sem_new_passenger);
	CloseHandle(taxi->pipe_write);
}

void send_response_taxi(taxi_t* taxi, int response)
{
	*taxi->message = response;
	ReleaseSemaphore(taxi->sem, 1, NULL);
}

void get_taxi_message(taxi_messages_t* tm, message_t* msg)
{
	WaitForSingleObject(tm->sem_can_read, INFINITE);

	msg->id = tm->messages->list[tm->messages->read_pos].id;
	msg->x = tm->messages->list[tm->messages->read_pos].x;
	msg->y = tm->messages->list[tm->messages->read_pos].y;
	msg->type = tm->messages->list[tm->messages->read_pos].type;
	msg->direction = tm->messages->list[tm->messages->read_pos].direction;
	_tcscpy_s(msg->passenger_name, MAX_TEXT, tm->messages->list[tm->messages->read_pos].passenger_name);

	tm->messages->read_pos = (tm->messages->read_pos + 1) % NUM_TAXI_MESSAGES;

	ReleaseSemaphore(tm->sem_can_write, 1, NULL);
}

taxi_t* create_taxi(int id)
{
	taxi_t* taxi = (taxi_t*) malloc(sizeof(taxi_t));
	if (taxi == NULL) {
		return NULL;
	}
	taxi->id = (int*) malloc(sizeof(int));
	if (taxi->id == NULL) {
		free(taxi);
		return NULL;
	}
	*taxi->id = id;

	taxi->direction = (int*)malloc(sizeof(int));
	if (taxi->direction == NULL) {
		free(taxi);
		return NULL;
	}
	taxi->state = (int*)malloc(sizeof(int));
	if (taxi->state == NULL) {
		free(taxi);
		return NULL;
	}
	taxi->pos = (point_t*)malloc(sizeof(point_t));
	if (taxi->pos == NULL) {
		free(taxi);
		return NULL;
	}
	taxi->message = NULL;
	taxi->pipe_write = INVALID_HANDLE_VALUE;

	return taxi;
}

void free_taxi(taxi_t* taxi)
{
	free(taxi->id);
	free(taxi->pos);
	free(taxi->direction);
	free(taxi->state);
	free(taxi);
}

taxi_t* get_taxi(settings_t* settings, int taxi_id)
{
	for (int i = 0; i < *settings->max_taxis; i++) {
		if (*settings->list_taxis[i].id == taxi_id) {
			return &settings->list_taxis[i];
		}
	}
	return NULL;
}


void update_taxi_pos(taxi_t* taxi, int x, int y, int direction, settings_t* settings)
{
	taxi->pos->x = x;
	taxi->pos->y = y;
	*taxi->direction = direction;
	send_response_taxi(taxi, TAXI_NOERROR);

	// Trigger Event to update Map
	SetEvent(settings->event_update_map);
}

/*
 
*/
passenger_t* get_passenger(settings_t *settings, TCHAR *passenger_name)
{
	for (int i = 0; i < *settings->max_passengers; i++) {
		if (settings->list_passengers[i].id[0] != '\0') {
			if (_tcsicmp(passenger_name, settings->list_passengers[i].id) == 0) {

				return &settings->list_passengers[i];
			}
		}
	}
	return NULL;
}

passenger_t* get_passenger_by_taxi_id(settings_t* settings, int taxi_id)
{
	for (int i = 0; i < *settings->max_passengers; i++) {
		if (settings->list_passengers[i].id[0] != '\0') {
			if (settings->list_passengers[i].taxi_id == taxi_id) {
				return &settings->list_passengers[i];
			}
		}
	}
	return NULL;
}

void add_taxi_interest(passenger_t* passenger, int taxi_id)
{
	passenger->list_taxis_interested[passenger->number_taxis_interested] = taxi_id;
	passenger->number_taxis_interested++;
}


void send_message_con_pass(settings_t* settings, TCHAR* message)
{
	DWORD n;
	WriteFile(settings->pipe_write_passengers, message, BUFFER_SIZE, &n, NULL);
}

void handle_message_register_taxi(message_t* msg, settings_t* settings)
{
	taxi_t* taxi = create_taxi(msg->id);
	create_taxi_ipc_resources(taxi);

	// Check if new taxis can register
	if (!settings->allow_taxis_registration) {
		_tprintf(L"\n: Not accepting new taxis. Taxi with id %d rejected\n> ", msg->id);
		send_response_taxi(taxi, TAXI_ERROR);
		free_taxi_ipc_resources(taxi);
		free_taxi(taxi);
		return;
	}

	// Check if system is full
	if (*settings->num_taxis == *settings->max_taxis) {
		_tprintf(L"\n: Can't accept more taxis (CenTaxi is full). Taxi with id %d rejected\n> ", msg->id);
		send_response_taxi(taxi, TAXI_ERROR);
		free_taxi_ipc_resources(taxi);
		free_taxi(taxi);
		return;
	}

	// Check if the initial map position is valid
	// TODO -- Check positoins and direction, new function

	// Send acknowledge back to the taxi
	send_response_taxi(taxi, TAXI_NOERROR);

	Sleep(200);
	taxi->pipe_write = open_taxi_named_pipe(*taxi->id);

	// Find free position for taxi
	for (int i = 0; i < *settings->max_taxis; i++) {
		if (*settings->list_taxis[i].id == -1) {
			// Update taxi information
			settings->list_taxis[i].pipe_write = taxi->pipe_write;
			settings->list_taxis[i].sem = taxi->sem;
			settings->list_taxis[i].sem_new_passenger = taxi->sem_new_passenger;
			settings->list_taxis[i].shm = taxi->shm;
			settings->list_taxis[i].message = taxi->message;
			settings->list_taxis[i].pos->x = msg->x;
			settings->list_taxis[i].pos->y = msg->y;
			settings->list_taxis[i].passenger_name = taxi->passenger_name;
			settings->list_taxis[i].passenger_pos = taxi->passenger_pos;
			*settings->list_taxis[i].direction = msg->direction;
			*settings->list_taxis[i].state = TAXI_STATE_FREE;
			*settings->list_taxis[i].id = *taxi->id;
			
			(*settings->num_taxis)++;
			break;
		}
	}


	// Trigger Event to update Map
	SetEvent(settings->event_update_map);
	_tprintf(L": New taxi (%d) registered.\n> ", *taxi->id);

	free_taxi(taxi);
}

void handle_message_unregister_taxi(message_t* msg, settings_t* settings)
{
	for (int i = 0; i < *settings->max_taxis; i++) {
		if (*settings->list_taxis[i].id == msg->id) {

			*settings->list_taxis[i].id = -1;
			free_taxi_ipc_resources(&settings->list_taxis[i]);
			
			// Check if there is a passenger in the taxi
			for (int i = 0; i < *settings->max_passengers; i++) {
				if (settings->list_passengers[i].id != '\0' && settings->list_passengers[i].taxi_id == msg->id) {
					// if found, remove the passenger
					remove_passenger(settings, &settings->list_passengers[i]);
					break;
				}
			}

			// Trigger Event to update Map
			SetEvent(settings->event_update_map);
			break;
		}
	}
}


void handle_message_new_pos_taxi(message_t* msg, settings_t* settings)
{
	taxi_t* taxi = get_taxi(settings, msg->id);
	if (taxi == NULL) {
		return; // Should not happen
	}

	node_t* aux = get_node(&settings->map, msg->x, msg->y);
	if (aux == NULL || aux->type == BLOCK) {
		send_response_taxi(taxi, TAXI_ERROR); // Can't move to that position
		return;
	}

	// Check if other taxi is in the node
	for (int i = 0; i < *settings->max_taxis; i++) {
		if (*settings->list_taxis[i].id != -1 && *settings->list_taxis[i].id != *taxi->id) {
			if (settings->list_taxis[i].pos->x == msg->x && settings->list_taxis[i].pos->y == msg->y) {
				// found a taxi in the positon where the taxi want to move
				// check taxi direction are opposite
				if (msg->direction == DIRECTION_UP && *settings->list_taxis[i].direction == DIRECTION_DOWN) {
					update_taxi_pos(taxi, msg->x, msg->y, msg->direction, settings);
				}
				else if (msg->direction == DIRECTION_DOWN && *settings->list_taxis[i].direction == DIRECTION_UP) {
					update_taxi_pos(taxi, msg->x, msg->y, msg->direction, settings);
				}
				else if (msg->direction == DIRECTION_LEFT && *settings->list_taxis[i].direction == DIRECTION_RIGHT) {
					update_taxi_pos(taxi, msg->x, msg->y, msg->direction, settings);
				}
				else if (msg->direction == DIRECTION_RIGHT && *settings->list_taxis[i].direction == DIRECTION_LEFT) {
					update_taxi_pos(taxi, msg->x, msg->y, msg->direction, settings);
				}
				else {
					send_response_taxi(taxi, TAXI_ERROR); // Can't move to that position
				}
				return;
			}
		}
	}
	update_taxi_pos(taxi, msg->x, msg->y, msg->direction, settings);
}

void handle_message_pickup_passenger(message_t* msg, settings_t* settings)
{
	taxi_t* taxi = get_taxi(settings, msg->id);

	passenger_t* passenger = get_passenger_by_taxi_id(settings, msg->id);
	if (passenger == NULL) {
		_tprintf(L": Passenger from taxi %d not found\n> ", msg->id);
		return;
	}
	_tprintf(L": Taxi %d picked up %s\n> ", msg->id, passenger->id);

	passenger->state = PASSENGER_STATE_WAITING_IN_TAXI;
	*taxi->state = TAXI_STATE_BUSY;

	TCHAR message[BUFFER_SIZE];
	_stprintf_s(message, BUFFER_SIZE, L"Taxi %d pickup passenger %s", *taxi->id, passenger->id);
	send_message_con_pass(settings, message);
}

void handle_message_delivered_passenger(message_t* msg, settings_t* settings)
{
	taxi_t* taxi = get_taxi(settings, msg->id);

	passenger_t* passenger = get_passenger_by_taxi_id(settings, msg->id);
	if (passenger == NULL) {
		_tprintf(L": Passenger from taxi %d not found\n> ", msg->id);
		return;
	}
	_tprintf(L": Taxi %d delivered %s\n> ", msg->id, passenger->id);

	*taxi->state = TAXI_STATE_FREE;

	TCHAR message[BUFFER_SIZE];
	_stprintf_s(message, BUFFER_SIZE, L"Taxi %d delivered passenger %s", *taxi->id, passenger->id);
	send_message_con_pass(settings, message);


	remove_passenger(settings, passenger);
}

void handle_message_interested(message_t* msg, settings_t* settings)
{
	_tprintf(L"\n: Interested received from taxi %d for passenger %s\n> ", msg->id, msg->passenger_name);

	taxi_t* taxi = get_taxi(settings, msg->id);
	if (taxi == NULL) {
		// Should not happen
		_tprintf(L": Taxi with ID %d not found\n> ", msg->id); // resrt console prompt
		return;
	}

	passenger_t *passenger = get_passenger(settings, msg->passenger_name);
	if (passenger == NULL) {
		_tprintf(L": Passenger %s not found\n> ", msg->passenger_name);
		return;
	}

	if (passenger->state != PASSENGER_STATE_WAITING) {
		_tprintf(L": Passenger %s already as a taxi assigned\n> ", msg->passenger_name);
		return;
	}

	add_taxi_interest(passenger, *taxi->id);

}



DWORD WINAPI wait_messages(LPVOID arg)
{
	settings_t* settings = (settings_t*)arg;
	taxi_messages_t* tm = settings->taxi_messages; // TODO should be save in settings to release in command exit

	if (tm == NULL) {
		_tprintf(L"[ERROR] It was not possible to start taxi resources!");
		exit(-1); // TODO should close differnetly, to release other shared resources
	}
	settings->taxi_messages = tm;


	while (1) {
		message_t msg;
		get_taxi_message(tm, &msg);

		// TODO - handle the message
		switch (msg.type) {
		case MSG_TYPE_REGISTER_TAXI:
			handle_message_register_taxi(&msg, settings);
			break;
		case MSG_TYPE_UNREGISTER_TAXI:
			handle_message_unregister_taxi(&msg, settings);
			break;
		case MSG_TYPE_NEW_POS_TAXI:
			handle_message_new_pos_taxi(&msg, settings);
			break;
		case MSG_TYPE_PICKUP_PASSENGER:
			handle_message_pickup_passenger(&msg, settings);
			break;
		case MSG_TYPE_DELIVERED_PASSENGER:
			handle_message_delivered_passenger(&msg, settings);
			break;
		case MSG_TYPE_INTERESTED:
			handle_message_interested(&msg, settings);
			break;
		}
	}
}


passenger_t* get_free_passenger(settings_t* settings)
{
	for (int i = 0; i < *settings->max_passengers; i++) {
		if (settings->list_passengers[i].id[0] == '\0') {
			return &settings->list_passengers[i];
		}
	}
	return NULL;
}

void remove_passenger(settings_t* settings, passenger_t* passenger)
{
	passenger->number_taxis_interested = 0;
	passenger->id[0] = '\0';
	passenger->taxi_id = -1;

	(*settings->num_passengers)--;
}

void shift_interested_taxis(int* list, int start_pos, int max)
{
	for (int i = start_pos; i < max - 1; i++) {
		list[i] = list[i + 1];
	}
}

void assign_passenger_to_taxi(settings_t *settings, passenger_t* passenger, taxi_t* taxi)
{
	// Update TAXI state information in Centaxi
	*taxi->state = TAXI_STATE_PICKING_PASSENGER;

	// Update passenger state
	passenger->state = PASSENGER_STATE_WAITING_FOR_TAXI;

	// Notify Taxi to get passenger
	DWORD type = MSG_TYPE_NEW_PASSENGER;
	DWORD n;
	WriteFile(taxi->pipe_write, &type, sizeof(DWORD), &n, NULL);
	WriteFile(taxi->pipe_write, &passenger->pos, sizeof(point_t), &n, NULL);
	WriteFile(taxi->pipe_write, &passenger->destiny, sizeof(point_t), &n, NULL);
	FlushFileBuffers(taxi->pipe_write);

	// 
	TCHAR message[BUFFER_SIZE];
	_stprintf_s(message, BUFFER_SIZE, L"\n: Taxi %d will pickup passenger %s\n> ", *taxi->id, passenger->id);
	send_message_con_pass(settings, message);
}

void CALLBACK check_for_interested_taxis(LPVOID arg)
{
	settings_t *settings = ((timer_arg_t*)arg)->settings;
	passenger_t *passenger = ((timer_arg_t*)arg)->passenger;
	free(arg);

	Sleep(settings->wait_for_taxis * 1000);

	_tprintf(L"\n: Checking taxis for passenger %s\n", passenger->id);

	passenger->state = NO_TAXI;

	for (int i = 0; i < passenger->number_taxis_interested; i++) {
		taxi_t* taxi = get_taxi(settings, passenger->list_taxis_interested[i]);
		if (taxi == NULL) {
			shift_interested_taxis(passenger->list_taxis_interested, i, *settings->max_taxis);
			passenger->number_taxis_interested--;
			continue; // Taxi might have left
		}
		// Check if taxi is still available
		if (*taxi->state != TAXI_STATE_FREE) {
			shift_interested_taxis(passenger->list_taxis_interested, i, *settings->max_taxis);
			passenger->number_taxis_interested--;
			continue; // not available
		}
	}

	if (passenger->number_taxis_interested == 0) {
		// no taxis available for the passenger
		TCHAR message[BUFFER_SIZE];
		_stprintf_s(message, BUFFER_SIZE, L": No taxis found for passenger %s\n", passenger->id);
		send_message_con_pass(settings, message);
		
		_tprintf(L": No taxis interested in passenger %s\n> ", passenger->id);

		remove_passenger(settings, passenger);

		// Update map
		SetEvent(settings->event_update_map);
		return;
	}

	// Choose a random taxi for the passenger
	int number = rand() % passenger->number_taxis_interested;
	taxi_t* taxi = get_taxi(settings, passenger->list_taxis_interested[number]);

	assign_passenger_to_taxi(settings, passenger, taxi);
	
	TCHAR message[BUFFER_SIZE];
	_stprintf_s(message, BUFFER_SIZE, L": Taxi %d assigned to passenger %s\n> ", *taxi->id, passenger->id);
	_tprintf(message);
	send_message_con_pass(settings, message);
}


void handle_new_passenger(settings_t* settings)
{
	TCHAR name[MAX_TEXT];
	TCHAR reply[BUFFER_SIZE];
	point_t destiny, pos;
	DWORD read_bytes;

	ReadFile(settings->pipe_read_passengers, name, sizeof(name), &read_bytes, NULL);
	ReadFile(settings->pipe_read_passengers, &pos, sizeof(point_t), &read_bytes, NULL);
	ReadFile(settings->pipe_read_passengers, &destiny, sizeof(point_t), &read_bytes, NULL);

	_tprintf(L"\n: New passenger request: %s (%d,%d) -> (%d,%d)\n> ", name, pos.x, pos.y, destiny.x, destiny.y);


	// Check if passenger exists
	for (int i = 0; i < *settings->max_passengers; i++) {
		if (_tcscmp(settings->list_passengers[i].id, name) == 0) {
			// Found passenger with same name
			_tcscpy_s(reply, BUFFER_SIZE, L"Passenger name already taken. Can't be used now.");
			send_message_con_pass(settings, reply);
			_tprintf(L"\n: Passenger name (%s) already taken. Can't accept this passenger\n> ", name);
			return;
		}
	}

	// Check for available space
	passenger_t* pass = get_free_passenger(settings);
	if (pass == NULL) {
		_tcscpy_s(reply, BUFFER_SIZE, L"Can't handle more passenger. Please try later.");
		send_message_con_pass(settings, reply);

		_tprintf(L"\n: Can't handle more passenger. Please try later.\n> ");
		return;
	}

	// Save passenger details
	_tcscpy_s(pass->id, MAX_TEXT, name);
	pass->destiny = destiny;
	pass->pos = pos;
	pass->state = PASSENGER_STATE_WAITING;
	pass->number_taxis_interested = 0;
	(*settings->num_passengers)++;

	// Check if list of taxis is initialized
	if (pass->list_taxis_interested == NULL) {
		pass->list_taxis_interested = malloc(sizeof(int) * *settings->max_taxis);
	}

	// Notify Map
	SetEvent(settings->event_update_map);

	// If Ok, notify Taxis about new passenger
	for (int i = 0; i < *settings->num_taxis; i++) {
		if (settings->list_taxis[i].id > 0) {
			settings->list_taxis[i].passenger_pos->x = pos.x;
			settings->list_taxis[i].passenger_pos->y = pos.y;
			_tcscpy_s(settings->list_taxis[i].passenger_name, MAX_TEXT, pass->id);
			ReleaseSemaphore(settings->list_taxis[i].sem_new_passenger, 1, NULL);
		}
	}

	// triger a Thread to wait for XX seconds to start choosing a taxi

	timer_arg_t *timer_arg = (timer_arg_t*) malloc(sizeof(timer_arg_t));
	timer_arg->passenger = pass;
	timer_arg->settings = settings;

	pass->timer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)check_for_interested_taxis, timer_arg, 0, NULL);
	if (pass->timer == NULL) {
		_tprintf(L"\nError creating thread for passenger %s\n> ", pass->id);
	}
}

DWORD WINAPI wait_passengers(LPVOID arg)
{
	settings_t* settings = (settings_t*)arg;
	TCHAR buffer[BUFFER_SIZE];
	OVERLAPPED ovl_read;
	OVERLAPPED ovl_write;

	settings->pipe_write_passengers = INVALID_HANDLE_VALUE;
	settings->pipe_read_passengers = INVALID_HANDLE_VALUE;
	settings->event_read = CreateEvent(NULL, FALSE, FALSE, NULL);
	ovl_read.hEvent = settings->event_read;
	settings->event_write = CreateEvent(NULL, FALSE, FALSE, NULL);
	ovl_write.hEvent = settings->event_write;

	for (;;) {
		// Read
		settings->pipe_read_passengers = CreateNamedPipe(PIPE_NAME_READ,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_WAIT | PIPE_TYPE_MESSAGE
			| PIPE_READMODE_MESSAGE, 1, sizeof(buffer), sizeof(buffer), 1000, NULL);

		if (settings->pipe_read_passengers == INVALID_HANDLE_VALUE) {
			_tprintf(L"Error creating pipe to read from ConPass\n");
			return 0;
		}

		// Write
		settings->pipe_write_passengers = CreateNamedPipe(PIPE_NAME_WRITE,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_WAIT | PIPE_TYPE_MESSAGE
			| PIPE_READMODE_MESSAGE, 1, sizeof(buffer), sizeof(buffer), 1000, NULL);
		if (settings->pipe_write_passengers == INVALID_HANDLE_VALUE) {
			_tprintf(L": Error creating pipe to read from ConPass\n");
			return 0;
		}

		ConnectNamedPipe(settings->pipe_read_passengers, &ovl_read);
		WaitForSingleObject(settings->event_read, INFINITE);

		ConnectNamedPipe(settings->pipe_write_passengers, &ovl_write);
		WaitForSingleObject(settings->event_write, INFINITE);

		_tprintf(L"\n: Connected to ConPass\n> ");

		// Inifinite cycle to wait for Conpass messages
		DWORD read_bytes;
		DWORD message_type;
		do {
			ReadFile(settings->pipe_read_passengers, &message_type, sizeof(DWORD), &read_bytes, NULL);

			if (message_type == MSG_TYPE_NEW_PASSENGER) {
				handle_new_passenger(settings);
			}
		} while (message_type != MSG_TYPE_EXIT);

		DisconnectNamedPipe(settings->pipe_read_passengers);
		CloseHandle(settings->pipe_read_passengers);
		SetEvent(settings->event_read);
		DisconnectNamedPipe(settings->pipe_write_passengers);
		CloseHandle(settings->pipe_write_passengers);
		SetEvent(settings->event_write);
	}
}


int create_ipc_resources_variables(settings_t* settings)
{
	settings->variables_shm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * 4, SHM_VARIABLES);
	if (settings->variables_shm == NULL) {
		return TAXI_ERROR;
	}
	register_ipc(SHM_VARIABLES, IPC_TYPE_FILE_MAPPING);

	int* aux = (int*)MapViewOfFile(settings->variables_shm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(int) * 4);
	if (aux == NULL) {
		CloseHandle(settings->variables_shm);
		return TAXI_ERROR;
	}

	settings->max_taxis = &aux[0];
	settings->num_taxis = &aux[1];
	settings->max_passengers = &aux[2];
	settings->num_passengers = &aux[3];

	return TAXI_NOERROR;
}

int create_ipc_resources_taxis_list(settings_t* settings, int max_taxis)
{
	// Taxi
	DWORD size = sizeof(taxi_shm_t) * max_taxis;
	settings->taxi_shm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(taxi_shm_t) * max_taxis, SHM_TAXI_LIST);
	if (settings->taxi_shm == NULL) {
		_tprintf(L"It was not possible to create shared memory for list of taxis.\n Error: %s\n", get_last_error());
		return TAXI_ERROR;
	}
	register_ipc(SHM_TAXI_LIST, IPC_TYPE_FILE_MAPPING);
	settings->list_taxis_shm = (taxi_shm_t*)MapViewOfFile(settings->taxi_shm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(taxi_shm_t) * max_taxis);
	if (settings->list_taxis_shm == NULL) {

		_tprintf(L"It was not possible to map shared memory for list of taxis.\n Error:%s\n", get_last_error());

		CloseHandle(settings->taxi_shm);
		return TAXI_ERROR;
	}
	settings->list_taxis = (taxi_t*) malloc(sizeof(taxi_t) * max_taxis);
	
	if (settings->list_taxis == NULL) {
		_tprintf(L"It was not possible allocate memory to list of taxis.\n Error:%s\n", get_last_error());
		UnmapViewOfFile(settings->list_taxis_shm);
		CloseHandle(settings->taxi_shm);
		return TAXI_ERROR;
	}

	for (int i = 0; i < max_taxis; i++) {
		settings->list_taxis_shm[i].id = -1;
		taxi_t* t = &settings->list_taxis[i];
		if (t != NULL) {
			t->pos = &(settings->list_taxis_shm[i].pos);
			t->direction = &(settings->list_taxis_shm[i].direction);
			t->id = &(settings->list_taxis_shm[i].id);  // strange warning
			t->state = &(settings->list_taxis_shm[i].state);
			t->message = NULL;
		}
	}
	return TAXI_NOERROR;
}

int create_ipc_resources_passengers_list(settings_t *settings, int max_passengers)
{
	// PAssengers
	settings->passenger_shm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(passenger_t) * max_passengers, SHM_PASSENGERS_LIST);
	if (settings->passenger_shm == NULL) {

		_tprintf(L"[Error] It was not possible to create windows objects.\n Error: %s\n", get_last_error());
		return TAXI_ERROR;
	}
	register_ipc(SHM_PASSENGERS_LIST, IPC_TYPE_FILE_MAPPING);

	settings->list_passengers = (passenger_t*)MapViewOfFile(settings->passenger_shm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(passenger_t) * max_passengers);
	if (settings->list_passengers == NULL) {

		_tprintf(L"[Error] It was not possible to create shared memory map \n Error:%s\n", get_last_error());
		CloseHandle(settings->passenger_shm);
		return TAXI_ERROR;
	}

	return TAXI_NOERROR;
}

void free_ipc_resources(settings_t* settings)
{
	CloseHandle(settings->passenger_shm);
	UnmapViewOfFile(settings->list_passengers);

	CloseHandle(settings->taxi_shm);
	UnmapViewOfFile(settings->list_taxis);
	free(settings->list_taxis);

	CloseHandle(settings->variables_shm);
	UnmapViewOfFile(settings->max_taxis);

	CloseHandle(settings->event_update_map);
}
