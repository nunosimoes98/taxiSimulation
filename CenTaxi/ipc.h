#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#include "data.h"
#include "../ConTaxiIPC/data.h"



int create_taxi_ipc_resources(taxi_t* taxi);
void free_taxi_ipc_resources(taxi_t* taxi);
void send_response_taxi(taxi_t* taxi, int response);


taxi_t* get_taxi(settings_t* settings, int taxi_id);

void get_taxi_message(taxi_messages_t* tm, message_t* msg);

void handle_message_register_taxi(message_t* msg, settings_t* settings);
void handle_message_unregister_taxi(message_t* msg, settings_t* settings);
void handle_message_new_pos_taxi(message_t* msg, settings_t* settings);
void handle_message_pickup_passenger(message_t* msg, settings_t* settings);
void handle_message_delivered_passenger(message_t* msg, settings_t* settings);
void handle_message_interested(message_t* msg, settings_t* settings);
DWORD WINAPI wait_messages(LPVOID arg);

DWORD WINAPI wait_passengers(LPVOID arg);

int create_ipc_resources_variables(settings_t* settings);
int create_ipc_resources_taxis_list(settings_t* settings, int max_taxis);
int create_ipc_resources_passengers_list(settings_t *settings, int max_passengers);
void free_ipc_resources(settings_t *settings);

void remove_passenger(settings_t* settings, passenger_t* passenger);