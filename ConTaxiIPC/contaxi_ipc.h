// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CONTAXIIPC_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CONTAXIIPC_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CONTAXIIPC_EXPORTS
#define CONTAXIIPC_API __declspec(dllexport)
#else
#define CONTAXIIPC_API __declspec(dllimport)
#endif

#include "data.h"


#define IPC_TYPE_MUTEX   1
//Critical Section = 2
#define IPC_TYPE_SEM     3
#define IPC_TYPE_EVENT   4
//o Waitable Timer = 5
#define IPC_TYPE_FILE_MAPPING       6
#define IPC_TYPE_VIEW_FILE_MAPPING  7
#define IPC_TYPE_NAMED_PIPE         8

typedef void (*function_log_ipc_t)(TCHAR*);
typedef void (*function_register_ipc_t)(TCHAR*, int);

extern CONTAXIIPC_API function_log_ipc_t log_ipc;
extern CONTAXIIPC_API function_register_ipc_t register_ipc;
extern CONTAXIIPC_API HINSTANCE h_ipc_dll;

CONTAXIIPC_API int load_log_ipc_dll();
CONTAXIIPC_API void free_log_ipc_dll();

CONTAXIIPC_API node_t* get_node(map_t* map, int x, int y);
CONTAXIIPC_API point_t null_point();

CONTAXIIPC_API void write_taxi_message(taxi_messages_t* tm, message_t* msg);

CONTAXIIPC_API int wait_taxi_response(taxi_response_t* tr);
CONTAXIIPC_API void wait_taxi_new_passenger(taxi_response_t* tr);

CONTAXIIPC_API taxi_messages_t* create_ipc_taxi_messages();
CONTAXIIPC_API void free_ipc_taxi_messages(taxi_messages_t* tm);


CONTAXIIPC_API HANDLE create_taxi_response_sem(int taxi_id);
CONTAXIIPC_API HANDLE create_taxi_sem_new_passenger(int taxi_id);
CONTAXIIPC_API HANDLE create_taxi_response_shm(int taxi_id);

CONTAXIIPC_API HANDLE create_taxi_named_pipe(int taxi_id, HANDLE *event, OVERLAPPED *ovl);
CONTAXIIPC_API HANDLE open_taxi_named_pipe(int taxi_id);


CONTAXIIPC_API taxi_response_t* create_ipc_taxi_response(int taxi_id);
CONTAXIIPC_API void free_ipc_taxi_response(taxi_response_t* tr);


CONTAXIIPC_API map_t* init_ipc_map();
CONTAXIIPC_API void free_ipc_map(map_t* map);
