#define WIN32_LEAN_AND_MEAN      /* */      // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <stdlib.h>
#include <Winreg.h>
#include <commdlg.h>
#include <ShlObj.h>

#include "data.h"
#include "../Common/common.h"
#include "../ConTaxiIPC/contaxi_ipc.h"


HBITMAP load_bmp(TCHAR* folder, TCHAR* filename, HINSTANCE instance)
{
	TCHAR buffer[_MAX_PATH];
	_tcscpy_s(buffer, _MAX_PATH, folder);
	_tcscat_s(buffer, _MAX_PATH, L"\\");
	_tcscat_s(buffer, _MAX_PATH, filename);

	HANDLE bmp = LoadImage(GetModuleHandle(NULL), buffer, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	return bmp;
}

void unload_bitmaps(settings_t* settings)
{
	DeleteObject(settings->img_taxi[DIRECTION_UP]);
	DeleteObject(settings->img_taxi[DIRECTION_DOWN]);
	DeleteObject(settings->img_taxi[DIRECTION_LEFT]);
	DeleteObject(settings->img_taxi[DIRECTION_RIGHT]);
	DeleteObject(settings->img_taxi_busy[DIRECTION_UP]);
	DeleteObject(settings->img_taxi_busy[DIRECTION_DOWN]);
	DeleteObject(settings->img_taxi_busy[DIRECTION_LEFT]);
	DeleteObject(settings->img_taxi_busy[DIRECTION_RIGHT]);
	DeleteObject(settings->img_taxi_free[DIRECTION_UP]);
	DeleteObject(settings->img_taxi_free[DIRECTION_DOWN]);
	DeleteObject(settings->img_taxi_free[DIRECTION_LEFT]);
	DeleteObject(settings->img_taxi_free[DIRECTION_RIGHT]);
	DeleteObject(settings->img_building);
	DeleteObject(settings->img_road[ROAD_LEFT]);
	DeleteObject(settings->img_road[ROAD_CORNER_LT]);
	DeleteObject(settings->img_road[ROAD_TOP]);
	DeleteObject(settings->img_road[ROAD_CORNER_RT]);
	DeleteObject(settings->img_road[ROAD_RIGHT]);
	DeleteObject(settings->img_road[ROAD_CORNER_RB]);
	DeleteObject(settings->img_road[ROAD_BOTTOM]);
	DeleteObject(settings->img_road[ROAD_CORNER_LB]);
	DeleteObject(settings->img_road[ROAD_CROSS]);
	DeleteObject(settings->img_road[ROAD_HORIZONTAL]);
	DeleteObject(settings->img_road[ROAD_VERTICAL]);
}

int load_bitmaps_template(settings_t* settings, HINSTANCE instance, TCHAR* template_folder)
{
	settings->img_taxi[DIRECTION_UP] = load_bmp(template_folder, L"taxi-up.png", instance);
	settings->img_taxi[DIRECTION_DOWN] = load_bmp(template_folder, L"taxi-down.png", instance);
	settings->img_taxi[DIRECTION_LEFT] = load_bmp(template_folder, L"taxi-left.png", instance);
	settings->img_taxi[DIRECTION_RIGHT] = load_bmp(template_folder, L"taxi-right.png", instance);

	settings->img_taxi_busy[DIRECTION_UP] = load_bmp(template_folder, L"taxi-up-passenger.bmp", instance);
	settings->img_taxi_busy[DIRECTION_DOWN] = load_bmp(template_folder, L"taxi-down-passenger.bmp", instance);
	settings->img_taxi_busy[DIRECTION_LEFT] = load_bmp(template_folder, L"taxi-left-passenger.bmp", instance);
	settings->img_taxi_busy[DIRECTION_RIGHT] = load_bmp(template_folder, L"taxi-right-passenger.bmp", instance);

	settings->img_taxi_free[DIRECTION_UP] = load_bmp(template_folder, L"taxi-up-free.bmp", instance);
	settings->img_taxi_free[DIRECTION_DOWN] = load_bmp(template_folder, L"taxi-down-free.bmp", instance);
	settings->img_taxi_free[DIRECTION_LEFT] = load_bmp(template_folder, L"taxi-left-free.bmp", instance);
	settings->img_taxi_free[DIRECTION_RIGHT] = load_bmp(template_folder, L"taxi-right-free.bmp", instance);

	settings->img_building = load_bmp(template_folder, L"building.bmp", instance);

	settings->img_road[ROAD_LEFT] = load_bmp(template_folder, L"road-semi-cross-2.bmp", instance);
	settings->img_road[ROAD_CORNER_LT] = load_bmp(template_folder, L"road-corner-1.bmp", instance);
	settings->img_road[ROAD_TOP] = load_bmp(template_folder, L"road-semi-cross-3.bmp", instance);
	settings->img_road[ROAD_CORNER_RT] = load_bmp(template_folder, L"road-corner-2.bmp", instance);
	settings->img_road[ROAD_RIGHT] = load_bmp(template_folder, L"road-semi-cross-4.bmp", instance);
	settings->img_road[ROAD_CORNER_RB] = load_bmp(template_folder, L"road-corner-3.bmp", instance);
	settings->img_road[ROAD_BOTTOM] = load_bmp(template_folder, L"road-semi-cross-1.bmp", instance);
	settings->img_road[ROAD_CORNER_LB] = load_bmp(template_folder, L"road-corner-4.bmp", instance);
	settings->img_road[ROAD_CROSS] = load_bmp(template_folder, L"road-cross.bmp", instance);
	settings->img_road[ROAD_HORIZONTAL] = load_bmp(template_folder, L"road-horizontal.bmp", instance);
	settings->img_road[ROAD_VERTICAL] = load_bmp(template_folder, L"road-vertical.bmp", instance);

	settings->img_passenger = load_bmp(template_folder, L"passenger.bmp", instance);

	return TAXI_NOERROR;
}


INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED) SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
}

int GetFolder(TCHAR* folderPath, TCHAR* szCaption, HWND hWnd, TCHAR* initialPath)
{
	int retVal = ERROR;

	TCHAR currentPath[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, currentPath);
	_tcscat_s(currentPath, _MAX_PATH, L"\\icons");

	// The BROWSEINFO struct tells the shell 
	// how it should display the dialog.
	BROWSEINFO bi;
	memset(&bi, 0, sizeof(bi));

	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.hwndOwner = hWnd;
	bi.lpszTitle = szCaption;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM) currentPath;

	// must call this if using BIF_USENEWUI
	OleInitialize(NULL);

	// Show the dialog and get the itemIDList for the 
	// selected folder.
	LPITEMIDLIST pIDL = SHBrowseForFolder(&bi);

	if (pIDL != NULL)
	{
		// Create a buffer to store the path, then 
		// get the path.
		TCHAR buffer[_MAX_PATH] = { '\0' };
		if (SHGetPathFromIDList(pIDL, buffer) != 0)
		{
			// Set the string value.
			_tcscpy_s(folderPath, _MAX_PATH, buffer);
			retVal = ERROR_SUCCESS;
		}

		// free the item id list
		CoTaskMemFree(pIDL);
	}

	OleUninitialize();

	return retVal;
}

void load_icon_set(HWND hWnd, settings_t* settings, HINSTANCE instance)
{
	// Open Dialog to select folder
	TCHAR folder[_MAX_PATH] = { '\0' };
	TCHAR currentPath[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, currentPath);
	_tcscat_s(currentPath, _MAX_PATH, L"\\icons");

	GetFolder(folder, L"Select Template Folder", hWnd, currentPath);

	// Validate folder content

	// Save new setting in registry
	HKEY key;
	LSTATUS status = RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\MapTaxi", &key);
	if (status != ERROR_SUCCESS) {
		// TODO - show message say settings not saved
	}

	status = RegSetValueEx(key, L"TemplateFolder", 0, REG_SZ,
		(LPBYTE)folder, (DWORD)(_tcslen(folder) + 1) * 2);

	if (status != ERROR_SUCCESS) {
		// TODO - show message say settings not saved
	}

	RegCloseKey(key);

	// Unload bitmaps
	unload_bitmaps(settings);

	// Load new template
	load_bitmaps_template(settings, instance, folder);
}

int get_icon_set_from_registry(settings_t* settings, HINSTANCE instance)
{
	HKEY key;

	LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\MapTaxi", 0, KEY_READ, &key);
	if (status != ERROR_SUCCESS) {
		return TAXI_ERROR; // No key set
	}

	TCHAR templateFolder[_MAX_PATH];
	DWORD size = _MAX_PATH;
	status = RegQueryValueEx(key, L"TemplateFolder", NULL, NULL, (LPBYTE) templateFolder, &size);
	if (status != ERROR_SUCCESS) {
		return TAXI_ERROR; // No key set
	}

	RegCloseKey(key);

	// Load template previously saved
	load_bitmaps_template(settings, instance, templateFolder);

	return TAXI_NOERROR;
}


int load_bitmaps(settings_t* settings, HINSTANCE instance)
{
	if (get_icon_set_from_registry(settings, instance)) {
		return TAXI_NOERROR;
	}

	TCHAR currentPath[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, currentPath);
	_tcscat_s(currentPath, _MAX_PATH, L"\\icons\\default");

	return load_bitmaps_template(settings, instance, currentPath);
}



int valid_template_folder(TCHAR *path)
{
	// check if all files required exist in folder
	// TODO
	return TAXI_NOERROR;
}

int is_node_road_cross(map_t* map, int x, int y)
{
	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x - 1, y);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	node_t* n3 = get_node(map, x, y + 1);
	if (n3 == NULL || n3->type != ROAD) {
		return 0;
	}

	node_t* n4 = get_node(map, x, y - 1);
	if (n4 == NULL || n4->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_horizontal(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x - 1, y);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_vertical(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x, y + 1);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y - 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_left(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y + 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	node_t* n3 = get_node(map, x, y - 1);
	if (n3 == NULL || n3->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_right(map_t* map, int x, int y) {

	node_t* n1 = get_node(map, x - 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y + 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	node_t* n3 = get_node(map, x, y - 1);
	if (n3 == NULL || n3->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_top(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x - 1, y);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	node_t* n3 = get_node(map, x, y + 1);
	if (n3 == NULL || n3->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_bottom(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x - 1, y);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	node_t* n3 = get_node(map, x, y - 1);
	if (n3 == NULL || n3->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_corner_lt(map_t* map, int x, int y) {

	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y + 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_corner_lb(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x + 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y - 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_corner_rt(map_t* map, int x, int y) {

	node_t* n1 = get_node(map, x - 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y + 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	return 1;
}

int is_node_road_corner_rb(map_t* map, int x, int y) {
	node_t* n1 = get_node(map, x - 1, y);
	if (n1 == NULL || n1->type != ROAD) {
		return 0;
	}

	node_t* n2 = get_node(map, x, y - 1);
	if (n2 == NULL || n2->type != ROAD) {
		return 0;
	}

	

	return 1;
}

HBITMAP get_bitmap(settings_t *settings, node_t *node)
{
	if (node->type == BLOCK) {
		return settings->img_building;
	}

	// check 11 cases
	if (is_node_road_cross(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_CROSS];
	}
	if (is_node_road_horizontal(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_HORIZONTAL];
	}
	if (is_node_road_vertical(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_VERTICAL];
	}
	if (is_node_road_left(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_LEFT];
	}
	if (is_node_road_right(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_RIGHT];
	}
	if (is_node_road_top(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_TOP];
	}
	if (is_node_road_bottom(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_BOTTOM];
	}
	if (is_node_road_corner_lt(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_CORNER_LT];
	}
	if (is_node_road_corner_lb(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_CORNER_LB];
	}
	if (is_node_road_corner_rt(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_CORNER_RT];
	}
	if (is_node_road_corner_rb(settings->map, node->node.x, node->node.y)) {
		return settings->img_road[ROAD_CORNER_RB];
	}

	return INVALID_HANDLE_VALUE;
}

HBITMAP get_taxi_bitmap(settings_t *settings, taxi_t* taxi)
{
	switch (taxi->state) {
	case TAXI_STATE_FREE: return settings->img_taxi_free[taxi->direction];
	case TAXI_STATE_PICKING_PASSENGER: return settings->img_taxi[taxi->direction];
	case TAXI_STATE_BUSY: return settings->img_taxi_busy[taxi->direction];
	}
	return settings->img_taxi_free[taxi->direction];
}

HBITMAP get_passenger_bitmap(settings_t* settings, passenger_t* passenger)
{
	return settings->img_passenger;
}

void create_map_bitmap(settings_t* settings)
{
	HDC hdc = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(hdc);

	settings->img_map_background = CreateCompatibleBitmap(hdc, *settings->map->width, *settings->map->height);
	SelectObject(memDC, settings->img_map_background);

	HDC auxDC = CreateCompatibleDC(memDC);

	for (int x = 0; x < *settings->map->width; x++){
		for (int y = 0; y < *settings->map->height; y++) {
			node_t *node = get_node(settings->map, x, y);
			HBITMAP bmp = get_bitmap(settings, node);
			SelectObject(auxDC, bmp);
			BitBlt(memDC, x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, auxDC, 0, 0, SRCCOPY);
		}
	}

	DeleteDC(auxDC);
}