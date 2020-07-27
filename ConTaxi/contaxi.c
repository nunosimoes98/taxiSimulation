#include <stdio.h>
#include <fcntl.h>
#include <tchar.h>
#include <stdlib.h>
#include <io.h>
#include <math.h>

#include "data.h"
#include "../Console/Console.h"
#include "../ConTaxiIPC/contaxi_ipc.h"
#include "../Common/common.h"

message_t create_message(int type, taxi_t* taxi, TCHAR* passenger_name)
{
	message_t msg;

	msg.type = type;
	msg.x = taxi->pos.x;
	msg.y = taxi->pos.y;
	msg.direction = taxi->direction;
	msg.id = taxi->id;
	_tcscpy_s(msg.passenger_name, MAX_TEXT, passenger_name);

	return msg;
}

message_t create_simple_message(int type, taxi_t* taxi)
{
	return create_message(type, taxi, L"\0");
}

int create_ipc_resources(settings_t* settings)
{
	settings->tm = create_ipc_taxi_messages();
	if (settings->tm == NULL) {
		_tprintf(L"Error opening taxis message buffer: %s", get_last_error());
		return TAXI_ERROR;
	}
	settings->tr = create_ipc_taxi_response(settings->taxi.id);
	if (settings->tr == NULL) {
		_tprintf(L"Error creating taxi IPC resources: %s", get_last_error());
		return TAXI_ERROR;
	}
	return TAXI_NOERROR;
}

void free_ipc_resources(settings_t* settings)
{
	free_ipc_taxi_messages(settings->tm);
	free_ipc_taxi_response(settings->tr);
}

void close_app(settings_t* settings)
{
	CloseHandle(settings->thread_new_passenger);
	CloseHandle(settings->thread_taxi_move);
	CloseHandle(settings->thread_wait_for_passengers);

	message_t msg = create_simple_message(MSG_TYPE_UNREGISTER_TAXI, &settings->taxi);
	write_taxi_message(settings->tm, &msg);
	_tprintf(L"Taxi unregistered with success.\n");

	// TODO - close name pipes

	free_ipc_map(settings->map);
	free_ipc_resources(settings);

	exit(EXIT_SUCCESS);
}

void command_exit(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;
	
	// kill threads
	TerminateThread(settings->thread_taxi_move, EXIT_SUCCESS);
	TerminateThread(settings->thread_new_passenger, EXIT_SUCCESS);
	TerminateThread(settings->thread_wait_for_passengers, EXIT_SUCCESS);

	close_app(settings);
}

void command_brake(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;
	if (settings->taxi.speed <= 0) {
		_tprintf(L"Taxi already stopped. Can't brake more.");
		return;
	}
	settings->taxi.speed -= 0.5;
}

void command_get(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (args[0] == '\0') {
		_tprintf(L"No passenger name was given\n");
		_tprintf(L"Example:\n");
		_tprintf(L"> get nuno\n");
		return;
	}

	message_t msg = create_message(MSG_TYPE_INTERESTED, &settings->taxi, args);
	write_taxi_message(settings->tm, &msg);
	_tprintf(L"Request sent to CenTaxi.\nIf you are assigned to the passenger you will be notified.\n");
}

void command_auto(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (settings->taxi.pickup_mode == AUTO_PICKUP) {
		settings->taxi.pickup_mode = MANUAL_PICKUP;
		_tprintf(L"Auto pickup for passenger mode disabled.\n");
	}
	else {
		settings->taxi.pickup_mode = AUTO_PICKUP;
		_tprintf(L"Auto pickup for passenger mode enabled.\n");
	}
}

void command_speedup(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;
	if (settings->taxi.speed >= MAX_SPEED) {
		_tprintf(L"Max speed reached. Can't accelerate more");
		return;
	}
	settings->taxi.speed += 0.5;
}

void command_range(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (args[0] == '\0') {
		_tprintf(L"No range was given\n");
		_tprintf(L"Example:\n");
		_tprintf(L"> range 10\n");
		return;
	}

	int range = _tstoi(args);
	if (range <= 0) {
		_tprintf(L"Invalid range. Must be a positive number.\n");
		return;
	}
	settings->taxi.pickup_distance = range;
	_tprintf(L"New range set to %d, to pickup passengers.\n", range);
}



commands_t* register_commands()
{
	commands_t* list = init_commands(10);
	if (list == NULL) {
		return NULL;
	}
	// TODO correct the help messages
	register_command(list, L"exit",
		L"EXIT",
		command_exit);
	register_command(list, L"brake",
		L"Decrease the speed of the taxi.",
		command_brake);
	register_command(list, L"get",
		L"User wants to get a passenger. Must provide the id of the passenger.",
		command_get);
	register_command(list, L"auto",
		L"Command for automatic driving.",
		command_auto);
	register_command(list, L"speedup",
		L"Increase the speed of taxi.",
		command_speedup);
	register_command(list, L"range",
		L"Defines the radius to automatically reply to passengers request within range.\nMust set the range as parameter.",
		command_range);

	return list;
}

point_t read_position()
{
	point_t p;

	_tprintf(L"Enter the initial coordinates\n");
	p.x = read_number(L"Coordinate X: ");
	p.y = read_number(L"Coordinate Y: ");
	return p;
}

int read_direction()
{
	int direction;
	_tprintf(L"%d - UP\n", DIRECTION_UP);
	_tprintf(L"%d - DOWN\n", DIRECTION_DOWN);
	_tprintf(L"%d - LEFT\n", DIRECTION_LEFT);
	_tprintf(L"%d - RIGHT\n", DIRECTION_RIGHT);

	for (;;) {
		direction = read_number(L"Enter the initial direction: ");
		if (direction == DIRECTION_UP || direction == DIRECTION_DOWN || direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT) {
			return direction;
		}
	}
}





int init_taxi(settings_t *settings)
{
	settings->taxi.id = read_number(L"Enter taxi Id: ");
	point_t initial_position = read_position();
	//point_t initial_position = { 1, 1 };
	settings->taxi.pos.x = initial_position.x;
	settings->taxi.pos.y = initial_position.y;

	settings->taxi.direction = read_direction();
	//settings->taxi.direction = DIRECTION_DOWN;
	settings->taxi.pickup_mode = MANUAL_PICKUP;
	settings->taxi.pickup_distance = INITIAL_RANGE;
	settings->taxi.passenger_pos = null_point();
	settings->taxi.destiny = null_point();
	settings->taxi.speed = INITIAL_SPEED;
	settings->taxi.has_passenger = FALSE;

	if (!create_ipc_resources(settings)) {
		_tprintf(L"It was not possible to create IPC resources. Taxi can't be started.\n Error:%s\n", get_last_error());
		return TAXI_ERROR;
	}

	return TAXI_NOERROR;
}

int register_taxi(settings_t* settings)
{
	// Send message to CenTaxi to register TAXI
	message_t msg = create_simple_message(MSG_TYPE_REGISTER_TAXI, &settings->taxi);

	write_taxi_message(settings->tm, &msg);
	int r = wait_taxi_response(settings->tr);

	if (r == TAXI_ERROR) {
		return TAXI_ERROR;
	}

	settings->pipe = create_taxi_named_pipe(settings->taxi.id, &settings->event, &settings->ovl);
	if (settings->pipe == INVALID_HANDLE_VALUE) {
		return TAXI_ERROR;
	}

	return r;
}

void move(point_t *next_pos, int next_direction, settings_t *settings)
{
	// Check with CenTaxi if can move

	message_t msg = create_simple_message(MSG_TYPE_NEW_POS_TAXI, &settings->taxi);
	msg.x = next_pos->x;
	msg.y = next_pos->y;
	msg.direction = next_direction;
	write_taxi_message(settings->tm, &msg);
	int response = wait_taxi_response(settings->tr);
	if (response == TAXI_NOERROR) {
		// OK to move, update position locally
		settings->taxi.direction = next_direction;
		settings->taxi.pos.x = next_pos->x;
		settings->taxi.pos.y = next_pos->y;
	}
	else {
		_tprintf(L"Not possible to move to (%d,%d). Waiting...\n", next_pos->x, next_pos->y);
	}
}

double calculate_distance(point_t* p1, point_t* p2)
{
	return sqrt(pow( (double)p1->x - (double)p2->x, 2) + pow((double)p1->y - (double)p2->y, 2));
}

/*
 * Returns the direction to where the taxi should be directed to go the nearest
 * point to arrive to the destiny. If there are 2 points with the same distance
 * it selects the first one found.
 */
int closest_position_to(point_t *next_pos, point_t *destiny, int current_direction, map_t *map)
{
	// Points around the current position
	// But doesn't allow the taxi to go back 180 degrees
	point_t points[3];
	int directions[3];
	int i = 0;
	if (current_direction != DIRECTION_LEFT) {
		directions[i] = DIRECTION_RIGHT;
		points[i].x = next_pos->x + 1;
		points[i++].y = next_pos->y;
	}
	if (current_direction != DIRECTION_RIGHT) {
		directions[i] = DIRECTION_LEFT;
		points[i].x = next_pos->x - 1;
		points[i++].y = next_pos->y;
	}
	if (current_direction != DIRECTION_UP) {
		directions[i] = DIRECTION_DOWN;
		points[i].x = next_pos->x;
		points[i++].y = next_pos->y + 1;
	}
	if (current_direction != DIRECTION_DOWN && i < 3) {
		// i is never 3 or higher, so warning was not correct
		directions[i] = DIRECTION_UP;
		points[i].x = next_pos->x;
		points[i++].y = next_pos->y - 1;
	}

	double closest_distance = INT_MAX;
	int next_direction = current_direction;
	for (int i = 0; i < 3; i++)
	{
		node_t* node = get_node(map, points[i].x, points[i].y);
		if (node->type == BLOCK) {
			continue; // Skip to the next point
		}

		double d = calculate_distance(next_pos, &points[i]);
		if (d < closest_distance) {
			int next_direction = directions[i];
			closest_distance = d;
		}
	}

	return next_direction;
}

int random_direction(point_t* next_pos, int current_direction, map_t* map)
{
	// Points around the current position
// But doesn't allow the taxi to go back 180 degrees
	point_t points[3];
	int directions[3];
	int i = 0;
	if (current_direction != DIRECTION_LEFT) {
		directions[i] = DIRECTION_RIGHT;
		points[i].x = next_pos->x + 1;
		points[i++].y = next_pos->y;
	}
	if (current_direction != DIRECTION_RIGHT) {
		directions[i] = DIRECTION_LEFT;
		points[i].x = next_pos->x - 1;
		points[i++].y = next_pos->y;
	}
	if (current_direction != DIRECTION_UP) {
		directions[i] = DIRECTION_DOWN;
		points[i].x = next_pos->x;
		points[i++].y = next_pos->y + 1;
	}
	if (current_direction != DIRECTION_DOWN && i < 3) {
		directions[i] = DIRECTION_UP;
		points[i].x = next_pos->x;
		points[i++].y = next_pos->y - 1;
	}

	// it can't exist dead end roads
	int valid_points[3] = { -1, -1, -1 };

	for (int i = 0; i < 3; i++)
	{
		node_t* node = get_node(map, points[i].x, points[i].y);
		if (node->type == BLOCK) {
			continue; // Skip to the next point
		}
		valid_points[i] = 1;
	}

	// select one randomly
	int n = rand() % 3;
	while (valid_points[n] == -1) {
		n++; // move to next
		n = n % 3; // avoid over flow
	}

	return directions[n];
}


int is_cross(point_t* p, map_t* map)
{
	int count = 0;
	node_t* aux = get_node(map, p->x, p->y - 1);
	if (aux != NULL && aux->type == ROAD) {
		count++;
	}
	aux = get_node(map, p->x, p->y + 1);
	if (aux != NULL && aux->type == ROAD) {
		count++;
	}
	aux = get_node(map, p->x + 1, p->y);
	if (aux != NULL && aux->type == ROAD) {
		count++;
	}
	aux = get_node(map, p->x - 1, p->y);
	if (aux != NULL && aux->type == ROAD) {
		count++;
	}
	if (count >= 3) {
		return 1;
	}
	return 0;
}

/*
 * Taxi is in a cross, must select the direction to move, depending
 * if has a passenger, needs to pickup a passenger or if just moving
 * around.
 */
int select_new_direction(point_t* next_pos, map_t* map, taxi_t* taxi)
{
	// Select a direction which is not the oposite of the current direction
	// exe: if moving UP it can't go DOWN.

	if (is_cross(next_pos, map)) {
		if (taxi->has_passenger) {
			// Going to destiny
			return closest_position_to(next_pos, &taxi->destiny, taxi->direction, map);
		}
		else if (taxi->passenger_pos.x != -1) {
			// Going to passenger position
			return closest_position_to(next_pos, &taxi->passenger_pos, taxi->direction, map);
		}
	}

	return random_direction(next_pos, taxi->direction, map);
}


point_t* next_position(taxi_t* taxi, map_t* map, int *next_direction)
{
	// Get next position
	node_t* aux = NULL;
	switch (taxi->direction) {
	case DIRECTION_UP: aux = get_node(map, taxi->pos.x, taxi->pos.y - 1); break;
	case DIRECTION_DOWN: aux = get_node(map, taxi->pos.x, taxi->pos.y + 1); break;
	case DIRECTION_RIGHT: aux = get_node(map, taxi->pos.x + 1, taxi->pos.y); break;
	case DIRECTION_LEFT: aux = get_node(map, taxi->pos.x - 1, taxi->pos.y); break;
	}
	if (aux == NULL || aux->type == BLOCK) {
		// It should not happen this
		// TODO - how to handle this??
		return &taxi->pos;
	}

	*next_direction = select_new_direction(&aux->node, map, taxi);
	
	return &aux->node;
}

int pos_equal(point_t* p1, point_t* p2)
{
	return p1->x == p2->x && p1->y == p2->y;
}

/*
 * Moves the taxi automatically
 */
DWORD WINAPI move_taxi(LPVOID arg)
{
	settings_t *settings = (settings_t*)arg;

	for (;;) {

		// Wait
		DWORD wait_in_mseconds = (DWORD) ((1.0 / settings->taxi.speed) * 1000);
		Sleep(wait_in_mseconds);

		int next_direction;
		point_t *pos = next_position(&settings->taxi, settings->map, &next_direction);
		move(pos, next_direction, settings);

		// Check if can drop passenger
		if (settings->taxi.has_passenger && pos_equal(&settings->taxi.pos, &settings->taxi.destiny)) {
			message_t msg = create_simple_message(MSG_TYPE_DELIVERED_PASSENGER, &settings->taxi);
			write_taxi_message(settings->tm, &msg);
			_tprintf(L"\n: Delivered passenger at (%d, %d)\n> ", settings->taxi.destiny.x, settings->taxi.destiny.y);
			settings->taxi.has_passenger = FALSE;
			settings->taxi.destiny = null_point();
			settings->taxi.passenger_pos = null_point();
		}

		// Check if can pickup passenger
		if (!settings->taxi.has_passenger && !is_null_point(&settings->taxi.destiny)) {
			// has a destiny and no passenger.
			// Check if arrived to the passenger pos
			if (pos_equal(&settings->taxi.pos, &settings->taxi.passenger_pos)) {
				message_t msg = create_simple_message(MSG_TYPE_PICKUP_PASSENGER, &settings->taxi);
				write_taxi_message(settings->tm, &msg);
				settings->taxi.has_passenger = TRUE;
				_tprintf(L"\n: Picked up passenger at (%d, %d)\n> ", settings->taxi.passenger_pos.x, settings->taxi.passenger_pos.y);
			}
		}
		
	}
}

void handle_new_passenger(settings_t* settings)
{
	if (settings->taxi.pickup_mode == AUTO_PICKUP) {
		if (calculate_distance(&settings->taxi.pos, settings->tr->passenger_pos) <= settings->taxi.pickup_distance) {
			// send message to CenTaxi
			message_t msg = create_message(MSG_TYPE_INTERESTED, &settings->taxi, settings->tr->passenger_name);
			write_taxi_message(settings->tm, &msg);
			_tprintf(L"\n: New passenger \"%s\", within range, communicated to central pickup request.\n", settings->tr->passenger_name);
		}
		else{
			_tprintf(L"\n: New passenger \"%s\", but to far to pickup.\n", settings->tr->passenger_name);
		}
	}
	else {
		_tprintf(L"\: nNew passenger \"%s\", at (%d, %d).\n", settings->tr->passenger_name, settings->tr->passenger_pos->x, settings->tr->passenger_pos->y);
	}
	_tprintf(L"> ");
}

DWORD WINAPI wait_new_passenger(LPVOID arg)
{
	settings_t* settings = (settings_t*)arg;
	DWORD read_bytes;
	DWORD message_type;



	do{
		ReadFile(settings->pipe, &message_type, sizeof(DWORD), &read_bytes, NULL);
		if (read_bytes == 0) {
			break;
		}

		if (message_type == MSG_TYPE_NEW_PASSENGER) {

			ReadFile(settings->pipe, &settings->taxi.passenger_pos, sizeof(point_t), &read_bytes, NULL);
			ReadFile(settings->pipe, &settings->taxi.destiny, sizeof(point_t), &read_bytes, NULL);

		}
	} while (message_type != MSG_TYPE_EXIT);

	_tprintf(L"\nCenTaxi is closing, Taxi will also shutdown.\n");

	CloseHandle(settings->pipe);

	close_app(settings);
	return 0;
}

DWORD wait_for_passengers(LPVOID arg)
{
	settings_t* settings = (settings_t*)arg;

	for (;;) {
		// Sem waiting
		wait_taxi_new_passenger(settings->tr);

		handle_new_passenger(settings);
	}
}

/*
 * MAIN
 */
int _tmain(int argc, TCHAR* argv[])
{
#ifdef UNICODE
	int result = _setmode(_fileno(stdin), _O_WTEXT);
	result = _setmode(_fileno(stdout), _O_WTEXT);
#endif 

	settings_t settings;

#ifdef _DEBUG
		Sleep(2000); // To wait for IPC resources started from CenTaxi
#endif // DEBUG


	if (!load_log_ipc_dll()) {
		_tprintf(L"Error loading DLL for log IPC\n");
		return 0;
	}

	// Get Map from shared memory
	settings.map = init_ipc_map();
	if (!settings.map) {
		free_ipc_resources(&settings);
		return EXIT_FAILURE;
	}

	if (!init_taxi(&settings)) {
		free_ipc_resources(&settings);
		free_ipc_map(settings.map);
		return EXIT_FAILURE;
	}

	if (!register_taxi(&settings)) {
		_tprintf(L"\nNot possible to register taxi. Central might be full or not allowing new taxis.\n");
		free_ipc_resources(&settings);
		free_ipc_map(settings.map);
		return EXIT_SUCCESS;
	}
	_tprintf(L"Taxi registered with success.");


	// Thread to move taxi
	settings.thread_taxi_move = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)move_taxi, &settings, 0, NULL);

	// Thread to wait for new passeneger information
	settings.thread_new_passenger = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_new_passenger, &settings, 0, NULL);

	settings.thread_wait_for_passengers = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_for_passengers, &settings, 0, NULL);

	commands_t* commands = register_commands();

	_tprintf(L"Write \"help\" to list the available commands.\n");
	for(;;) {
		TCHAR* command = read_command();
		execute_command(commands, command, &settings);
	}

	return EXIT_SUCCESS;
}