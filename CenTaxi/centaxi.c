#include <stdio.h>
#include <wtypes.h>
#include <fcntl.h>
#include <tchar.h>
#include <io.h>
#include <Windows.h>


#include "data.h"
#include "constants.h"
#include "settings.h"
#include "map.h"
#include "ipc.h"
#include "../Common/common.h"

#include "../Console/Console.h"
#include "../ConTaxiIPC/contaxi_ipc.h"

void command_exit(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;
	DWORD type = MSG_TYPE_EXIT;
	DWORD n;

	// Notify taxis, CenTaxi is closing
	for (int i = 0; i < *settings->max_taxis; i++) {
		taxi_t* t = &settings->list_taxis[i];
		if (*t->id > 0) {
			WriteFile(t->pipe_write, &type, sizeof(DWORD), &n, NULL);
			FlushFileBuffers(t->pipe_write);
			free_taxi_ipc_resources(t);
		}
	}

	// kill threads 
	TerminateThread(settings->thread_wait_messages, 0);
	TerminateThread(settings->thread_wait_passengers, 0);

	// free shared resourcs
	free_ipc_taxi_messages(settings->taxi_messages);

	free_ipc_resources(settings);

	exit(EXIT_SUCCESS); // close app
}

void command_kick_taxi(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (args[0] == '\0') {
		_tprintf(L"No taxi id set.\n");
		_tprintf(L"Example:\n");
		_tprintf(L"> kick 4\n");
		return;
	}

	int taxi_id = _tstoi(args);
	if (taxi_id <= 0) {
		_tprintf(L"Invalid taxi id. Must be a positive number.\n");
		return;
	}

	taxi_t* taxi = get_taxi(settings, taxi_id);
	if (taxi == NULL) {
		_tprintf(L"Invalid taxi id. No taxi with id %d found.\n", taxi_id);
		return;
	}

	// Use name pipe to send shutdown message to taxi
	DWORD type = MSG_TYPE_EXIT;
	DWORD n;
	WriteFile(taxi->pipe_write, &type, sizeof(DWORD), &n, NULL);
	FlushFileBuffers(taxi->pipe_write);

	_tprintf(L"Sent request to unrigester taxi with id %d\n", taxi_id);
}

void command_list_taxis(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (*settings->num_taxis == 0) {
		_tprintf(L"No taxis registered\n");
		return;
	}

	_tprintf(L"\nList of taxis:\n");
	for (int i = 0; i < *settings->max_taxis; i++) {
		if (*settings->list_taxis[i].id > 0) {
			_tprintf(L"- %d - (State: ", *settings->list_taxis[i].id);
			switch (*settings->list_taxis[i].state) {
			case TAXI_STATE_BUSY:
				_tprintf(L"Taking passenger to destiny)\n");
				break;
			case TAXI_STATE_FREE:
				_tprintf(L"Free)\n");
				break;
			case TAXI_STATE_PICKING_PASSENGER:
				_tprintf(L"Picking up passenger)\n");
				break;
			}
		}
	}
}

void command_list_passengers(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (*settings->num_passengers == 0) {
		_tprintf(L"No passengers\n");
		return;
	}

	_tprintf(L"\nList of passengers:\n");
	for (int i = 0; i < *settings->max_passengers; i++) {
		if (settings->list_passengers[i].id[0] != '\0') {
			_tprintf(L"- %s - (State: ", settings->list_passengers[i].id);
			switch (settings->list_passengers[i].state) {
			case PASSENGER_STATE_WAITING:
				_tprintf(L"Waiting for taxi assignment)\n");
				break;
			case PASSENGER_STATE_WAITING_FOR_TAXI:
				_tprintf(L"Waiting for taxi %d)\n", settings->list_passengers[i].taxi_id);
				break;
			case PASSENGER_STATE_WAITING_IN_TAXI:
				_tprintf(L"In taxi %d)\n", settings->list_passengers[i].taxi_id);
				break;
			}
		}
	}
}

void command_stop_accept_taxis(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	settings->allow_taxis_registration = FALSE;
	_tprintf(L"Not allowing new taxis to register.\n");
}

void command_restart_accept_taxis(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	settings->allow_taxis_registration = TRUE;
	_tprintf(L"Allowing new taxis to register.\n");
}

void command_wait(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;

	if (args[0] == '\0') {
		_tprintf(L": No wait time ins seconds for taxis was given.\n");
		_tprintf(L": Example:\n");
		_tprintf(L": > wait 60\n");
		return;
	}

	int wait_time = _tstoi(args);
	if (wait_time <= 0) {
		_tprintf(L": Invalid time. Must be a positive number.\n");
		return;
	}
	settings->wait_for_taxis = wait_time;
	_tprintf(L": Waiting time for taxis set to %d seconds.\n", wait_time);

}

commands_t* register_commands()
{
	commands_t* list = init_commands(50);
	if (list == NULL) {
		return NULL;
	}
	register_command(list, L"exit",
		L"Shutdown CenTaxi and notifies Taxis",
		command_exit);
	register_command(list, L"kick",
		L"Kick out a Taxi  without passengers",
		command_kick_taxi);
	register_command(list, L"list-taxis",
		L"List Taxis",
		command_list_taxis);
	register_command(list, L"list-passengers",
		L"List Passengers",
		command_list_passengers);
	register_command(list, L"stop",
		L"Stop accepting Taxis",
		command_stop_accept_taxis);
	register_command(list, L"restart",
		L"Shutdown CenTaxi and notifies Taxis",
		command_restart_accept_taxis);
	register_command(list, L"wait",
		L"Set maximum waiting time for a passenger to wait for a taxi",
		command_wait);


	return list;
}



int _tmain(int argc, _TCHAR* argv[])
{

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif 

	if (!load_log_ipc_dll()) {
		_tprintf(L"Error loading DLL for log IPC\n");
		return 0;
	}

	// Assure only one instance is running
	// TODO

	// Read arguments from command line
	settings_t* settings = load_settings(argc, argv);
	if (settings == NULL) {
		_tprintf(L"Invalid arguments. Use -p or -t\n");
		free_log_ipc_dll();
		return 0;
	}

	// Read map file
	if (!load_map(settings)) {
		_tprintf(L"Problem loading the map file\n");
		free_ipc_resources(settings);
		free_log_ipc_dll();
		return 0;
	}

	settings->taxi_messages = create_ipc_taxi_messages();
	if (settings->taxi_messages == NULL) {
		free_ipc_map(&settings->map);
		free_ipc_resources(settings);
		free_log_ipc_dll();
		_tprintf(L"Problem loading IPC resources for taxi messages\n");
		return 0;
	}

	settings->event_update_map = CreateEvent(NULL, TRUE, FALSE, EVENT_MAP_UPDATE);
	if (settings->event_update_map == NULL) {
		free_ipc_map(&settings->map);
		free_ipc_resources(settings);
		free_log_ipc_dll();
		_tprintf(L"Problem Creating event for map updates\n");
		return 0;
	}
	register_ipc(EVENT_MAP_UPDATE, IPC_TYPE_EVENT);


	commands_t* list_commands = register_commands();
	if (list_commands == NULL) {
		_tprintf(L"Error loading commands. Aborting...\n");
		free_ipc_map(&settings->map);
		free_ipc_resources(settings);
		free_log_ipc_dll();
		return 0;
	}

	settings->thread_wait_messages = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_messages, settings, 0, NULL);

	settings->thread_wait_passengers = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_passengers, settings, 0, NULL);

	while (1) {
		TCHAR* command = read_command();
		execute_command(list_commands, command, settings);
	}
	return 0;
}
