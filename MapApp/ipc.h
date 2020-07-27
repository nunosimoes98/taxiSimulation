#pragma once

#include "data.h"

int create_ipc_resources_variables(settings_t* settings);
int create_ipc_resources_taxis_list(settings_t* settings);
int create_ipc_resources_passengers_list(settings_t* settings);
void free_ipc_resources(settings_t* settings);