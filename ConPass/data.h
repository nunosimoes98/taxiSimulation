#pragma once

#include "../ConTaxiIPC/data.h"

// Names are inverted from CenTaxi project
#define PIPE_NAME_READ   L"\\\\.\\pipe\\ConPassRead"
#define PIPE_NAME_WRITE  L"\\\\.\\pipe\\ConPassWrite"


typedef struct {
	point_t pos;
	TCHAR id[MAX_TEXT];
	point_t destiny;
} passenger_t;

typedef struct {

	HANDLE pipe_read;
	HANDLE pipe_write;
	HANDLE thread_wait_messages;

} settings_t;