#include <tchar.h>
#include <stdlib.h>
#include <strsafe.h>


#include "ipc.h"
#include "data.h"
#include "../ConTaxiIPC/contaxi_ipc.h"
#include "../Common/common.h"


int create_ipc_resources_variables(settings_t* settings)
{
	settings->variables_shm = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHM_VARIABLES);
	if (settings->variables_shm == NULL) {
		return TAXI_ERROR;
	}
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

int create_ipc_resources_taxis_list(settings_t* settings)
{
	// Taxi
	settings->taxi_shm = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHM_TAXI_LIST);
	if (settings->taxi_shm == NULL) {
		//printf(L"It was not possible to create windows objects.\n Error: %s\n", get_last_error());
		return TAXI_ERROR;
	}
	settings->list_taxis = (taxi_t*)MapViewOfFile(settings->taxi_shm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(taxi_t) * *settings->max_taxis);
	if (settings->list_taxis == NULL) {

		//_tprintf(L"It was not possible to create shared memory map \n Error:%s\n", get_last_error());

		CloseHandle(settings->taxi_shm);
		return TAXI_ERROR;
	}


	return TAXI_NOERROR;
}

int create_ipc_resources_passengers_list(settings_t* settings)
{
	// PAssengers
	settings->passenger_shm = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHM_PASSENGERS_LIST);
	if (settings->passenger_shm == NULL) {

		//_tprintf(L"[Error] It was not possible to create windows objects.\n Error: %s\n", get_last_error());
		return TAXI_ERROR;
	}
	settings->list_passengers = (passenger_t*)MapViewOfFile(settings->passenger_shm, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(passenger_t) * *settings->max_passengers);
	if (settings->list_passengers == NULL) {

		//_tprintf(L"[Erro] It was not possible to create shared memory map \n Error:%s\n", get_last_error());
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
	CloseHandle(settings->variables_shm);
	UnmapViewOfFile(settings->max_taxis);
}
