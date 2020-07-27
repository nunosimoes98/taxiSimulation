#include <stdio.h>
#include <wtypes.h>
#include <fcntl.h>
#include <tchar.h>
#include <io.h>
#include <Windows.h>
#include <tchar.h>

#include "../Console/Console.h"
#include "data.h"



point_t read_position()
{
	point_t p;

	_tprintf(L"Enter the initial coordinates\n");
	p.x = read_number(L"Coordinate X: ");
	p.y = read_number(L"Coordinate Y: ");
	return p;
}

void init_passenger(passenger_t* passenger) {

	TCHAR id[MAX_TEXT];

	read_text(L"Enter Passenger Id: ", id, MAX_TEXT);

	_tcscpy_s(passenger->id, MAX_TEXT, id);
	point_t initial_position = read_position();
	passenger->pos.x = initial_position.x;
	passenger->pos.y = initial_position.y;

	_tprintf(L"Where do you want to go?");

	point_t last_position = read_position();
	passenger->destiny.x = last_position.x;
	passenger->destiny.y = last_position.y;
}

void command_exit(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;
	DWORD type = MSG_TYPE_EXIT;
	DWORD n;

	WriteFile(settings->pipe_write, &type, sizeof(DWORD), &n, NULL);
	Sleep(200);

	// Close thread
	TerminateThread(settings->thread_wait_messages, 0);

	// Close named pipes
	CloseHandle(settings->pipe_write);
	CloseHandle(settings->pipe_read);
	
	exit(1); // close app
}

void command_new(TCHAR* args, void* var)
{
	settings_t* settings = (settings_t*)var;
	passenger_t passenger;
	DWORD n;
	DWORD type = MSG_TYPE_NEW_PASSENGER;

	init_passenger(&passenger);

	WriteFile(settings->pipe_write, &type, sizeof(DWORD), &n, NULL);
	WriteFile(settings->pipe_write, passenger.id, sizeof(passenger.id), &n, NULL);
	WriteFile(settings->pipe_write, &passenger.pos, sizeof(point_t), &n, NULL);
	WriteFile(settings->pipe_write, &passenger.destiny, sizeof(point_t), &n, NULL);
	FlushFileBuffers(settings->pipe_write);
}


commands_t* register_commands()
{
	commands_t* list = init_commands(50);

	register_command(list, L"exit",
		L"Shutdown CenTaxi and notifies Taxis",
		command_exit);
	register_command(list, L"new",
		L"Creates a new request for Taxi for a passenger.",
		command_new);

	return list;
}


DWORD WINAPI wait_messages(LPVOID arg)
{
	settings_t* settings = (settings_t*)arg;
	TCHAR buffer[BUFFER_SIZE];
	DWORD n;

	for (;;) {
		ReadFile(settings->pipe_read, buffer, sizeof(buffer), &n, NULL);
		if (n == 0) {
			break;
		}
		_tprintf(L"\n: %s\n> ", buffer);
	}
	_tprintf(L"\nLost communication with CenTaxi. Closing");

	CloseHandle(settings->pipe_write);
	CloseHandle(settings->pipe_read);

	exit(1); // close app
}

int init_named_pipes(settings_t* settings)
{
	if (!WaitNamedPipe(PIPE_NAME_WRITE, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(L"Error connecting to write pipe.\n");
		return 0;
	}
	settings->pipe_write = CreateFile(PIPE_NAME_WRITE, GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	Sleep(1000); // Cheat, it should not be done.

	if (!WaitNamedPipe(PIPE_NAME_READ, NMPWAIT_WAIT_FOREVER)) {
		_tprintf(L"Error connecting to read pipe.\n");
		return 0;
	}
	settings->pipe_read = CreateFile(PIPE_NAME_READ, GENERIC_READ, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	return 1;
}


int _tmain(int argc, _TCHAR* argv[])
{
	settings_t settings;
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif 

#ifdef _DEBUG
	Sleep(2000); // To wait for IPC resources started from CenTaxi
#endif // DEBUG

	if (!init_named_pipes(&settings)) {
		_tprintf(L"Error opening named pipes. Aborting...\n");
		return 0;
	}

	commands_t* list_commands = register_commands();
	if (list_commands == NULL) {
		_tprintf(L"Error loading commands. Aborting...\n");
		return 0;
	}

	settings.thread_wait_messages = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_messages, &settings, 0, NULL);

	_tprintf(L"Write \"help\" to list the available commands.\n");

	for(;;) {
		TCHAR* command = read_command();
		execute_command(list_commands, command, &settings);
	}
	return 0;
}
