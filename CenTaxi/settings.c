#include <tchar.h>
#include <stdio.h>
#include <wtypes.h>

#include "data.h"
#include "constants.h"
#include "ipc.h"


settings_t* init_settings() {
	settings_t* settings = malloc(sizeof(settings_t));
	if (settings == NULL) {
		return NULL;
	}

	create_ipc_resources_variables(settings);
	*settings->num_taxis = 0;
	*settings->num_passengers = 0;

	settings->map_filename = _tcsdup(TEXT(DEFAULT_MAP_FILENAME));

	settings->allow_taxis_registration = 1; // By default accept new taxis
	settings->wait_for_taxis = DEFAULT_WAIT_FOR_TAXIS;

	return settings;
}

void load_taxis(settings_t* settings, int max)
{
	*settings->max_taxis = max;
	create_ipc_resources_taxis_list(settings, max);
}

void load_default_settings_taxis(settings_t* settings)
{
	load_taxis(settings, DEFAULT_NUM_TAXIS);
}

void load_passengers(settings_t* settings, int max)
{
	*settings->max_passengers = max;
	create_ipc_resources_passengers_list(settings, max);

	for (int i = 0; i < max; i++) {
		settings->list_passengers[i].id[0] = '\0'; // Available
		settings->list_passengers[i].number_taxis_interested = 0;
		settings->list_passengers[i].list_taxis_interested = NULL;
		settings->list_passengers[i].timer = NULL;
	}
}

void load_default_settings_passengers(settings_t* settings)
{
	load_passengers(settings, DEFAULT_NUM_PASSENGERS);
}


settings_t* load_settings(int argc, TCHAR* argv[])
{
	// check if argument are valid, must be odd number
	if (argc % 2 == 0) {
		return NULL; // Invalid parameters
	}

	settings_t* settings = init_settings();
	if (argc == 1) {
		load_default_settings_taxis(settings);
		load_default_settings_passengers(settings);
		return settings;
	}

	int n = 1;
	while (n < argc) {
		// Passenger option
		if (_tcscmp(argv[n], L"-p") == 0) {
			int m = _tstoi(argv[n + 1]);
			if (m > 0) {
				load_passengers(settings, m);
			}
			else {
				// LOG TAXI_ERRORs
			}
		}
		// Taxi option
		else if (_tcscmp(argv[n], L"-t") == 0) {
			int m = _tstoi(argv[n + 1]);
			if (m > 0) {
				load_taxis(settings, m);
			}
			else {
				// LOG TAXI_ERRORs
			}
		}
		// Map file option
		else if (_tcscmp(argv[n], L"-m") == 0) {
			free(settings->map_filename);
			settings->map_filename = argv[n + 1];
		}

		n += 2;
	}

	// Check if no arguments were used for taxis or passengers
	// load the default values
	if (settings->max_taxis == 0) {
		load_default_settings_taxis(settings);
	}
	if (settings->max_passengers == 0) {
		load_default_settings_passengers(settings);
	}

	return settings;
}

