#pragma once

#include "data.h"

void load_icon_set(HWND hWnd, settings_t* settings, HINSTANCE instance);
int load_bitmaps(settings_t* settings, HINSTANCE instance);
HBITMAP get_bitmap(settings_t* settings, node_t* node);
HBITMAP get_taxi_bitmap(settings_t* settings, taxi_t* taxi);
HBITMAP get_passenger_bitmap(settings_t* settings, passenger_t *passenger);