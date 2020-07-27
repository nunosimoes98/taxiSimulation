// MapApp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "MapApp.h"

#include "data.h"
#include "ipc.h"
#include "../ConTaxiIPC/contaxi_ipc.h"
#include "../Common/common.h"
#include "image_resources.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
settings_t settings;
HANDLE hMutexSingleInstance;


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Error(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI wait_for_udpates(LPVOID arg)
{
    HWND hWnd = (HWND)arg;

    HANDLE event = CreateEvent(NULL, TRUE, FALSE, EVENT_MAP_UPDATE);
    if (event == NULL) {
        return FALSE;
    }
    register_ipc(EVENT_MAP_UPDATE, IPC_TYPE_EVENT);

    for (;;) {
        WaitForSingleObject(event, INFINITE);
        InvalidateRect(hWnd, NULL, 1); // force to update the window
        ResetEvent(event);
    }
    return FALSE;
}

void release_single_instance()
{
    ReleaseMutex(hMutexSingleInstance);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
    Sleep(2000); // To wait for IPC resources started from CenTaxi
#endif // DEBUG

    // Assure single instance
    // https://stackoverflow.com/questions/4191465/how-to-run-only-one-instance-of-application
    hMutexSingleInstance = OpenMutex(MUTEX_ALL_ACCESS, 0, SINGLE_INSTANCE_MUTEX_NAME);
    if (hMutexSingleInstance) {
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_ERROR_SINGLE_INSTANCE), NULL, About);
        return FALSE;
    }

    // Mutex doesn’t exist. This is the first instance so create the mutex.
    hMutexSingleInstance = CreateMutex(0, 0, SINGLE_INSTANCE_MUTEX_NAME);
    

    if (!load_log_ipc_dll()) {
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_ERROR_LOAD_IPC_DLL), NULL, About);
        release_single_instance();
        return 0;
    }

    // Load IPC resources
    settings.map = init_ipc_map();
    if (settings.map == NULL) {
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_ERROR_LOAD_MAP), NULL, About);
        release_single_instance();
        return FALSE;
    }

    // Load IPC Passengers
    create_ipc_resources_variables(&settings);
    create_ipc_resources_taxis_list(&settings);
    create_ipc_resources_passengers_list(&settings);


    // Load map
    load_bitmaps(&settings, hInstance);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAPAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        release_single_instance();
        return FALSE;
    }

    // Create trhead to wait for Events
    // TODO create thread
    //InvalidateRect(hWnd, NULL, 1); //vai gerar um paint

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAPAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    release_single_instance();
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAPAPP));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MAPAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAPAPP));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void updateMap(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    // TODO: Add any drawing code that uses hdc here...
    
    HDC memDC = CreateCompatibleDC(hdc);

    settings.img_map_background = CreateCompatibleBitmap(hdc, *settings.map->width * TILE_SIZE, *settings.map->height * TILE_SIZE);
    SelectObject(memDC, settings.img_map_background);

    HDC auxDC = CreateCompatibleDC(memDC);

    // Draw roads and buldings
    for (int x = 0; x < *settings.map->width; x++) {
        for (int y = 0; y < *settings.map->height; y++) {
            node_t* node = get_node(settings.map, x, y);
            HBITMAP bmp = get_bitmap(&settings, node);
            SelectObject(auxDC, bmp);
            BitBlt(memDC, x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, auxDC, 0, 0, SRCCOPY);
        }
    }

    // Add Taxis
    for (int t = 0; t < *settings.max_taxis; t++) {
        if (settings.list_taxis[t].id > 0) {  // TODO hack to solve -256 bug in taxi id
            taxi_t* taxi = &settings.list_taxis[t];
            HBITMAP taxi_bmp = get_taxi_bitmap(&settings, taxi);
            SelectObject(auxDC, taxi_bmp);

            BitBlt(memDC, taxi->pos.x * TILE_SIZE, taxi->pos.y * TILE_SIZE, TILE_SIZE, TILE_SIZE, auxDC, 0, 0, MERGECOPY);
        }
    }

    // Add passengers
    for (int p = 0; p < *settings.max_passengers; p++) {
        if (settings.list_passengers[p].id[0] != '\0' && settings.list_passengers[p].state != PASSENGER_STATE_WAITING_IN_TAXI) {
            passenger_t* passenger = &settings.list_passengers[p];
            HBITMAP passenger_bmp = get_passenger_bitmap(&settings, passenger);
            SelectObject(auxDC, passenger_bmp);

            BitBlt(memDC, passenger->pos.x * TILE_SIZE, passenger->pos.y * TILE_SIZE, TILE_SIZE, TILE_SIZE, auxDC, 0, 0, MERGECOPY);
        }
    }

    DeleteDC(auxDC);

    SelectObject(memDC, settings.img_map_background);

    BitBlt(hdc, 0, 0, GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CXSCREEN), memDC, 0, 0, SRCCOPY);

    

    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
}



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_for_udpates, hWnd, 0, NULL);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_LOADICONSET:
                load_icon_set(hWnd, &settings, hInst);
                InvalidateRect(hWnd, NULL, 1);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        updateMap(hWnd);
        break;
    case WM_ERASEBKGND:
        return (LRESULT)1; // Necessário para não piscar ao atualizar o ecrã
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

/**
 * Handler for Error box.
 **/
INT_PTR CALLBACK Error(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}