// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CONSOLE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CONSOLE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CONSOLE_EXPORTS
#define CONSOLE_API __declspec(dllexport)
#else
#define CONSOLE_API __declspec(dllimport)
#endif

#include <tchar.h>

#define CONSOLE_MAX_TEXT 100
#define CONSOLE_MAX_TEXT_HELP 300
#define CONSOLE_ERROR 0
#define CONSOLE_NOERROR 1
#define CONSOLE_EXECUTED 1
#define CONSOLE_NOT_EXECUTED 0


typedef struct {
    TCHAR name[CONSOLE_MAX_TEXT];
    TCHAR help[CONSOLE_MAX_TEXT_HELP];
    void (*function)(TCHAR* text, void* var);
} command_t;

typedef struct {
    command_t* list;
    int count;
    int max;
} commands_t;

CONSOLE_API void read_text(TCHAR* text, TCHAR* output, int size);
CONSOLE_API TCHAR* read_command();

// Function to read a integer from the console, give a text to show before hand
CONSOLE_API int read_number(TCHAR* text);

CONSOLE_API int execute_command(commands_t* list, TCHAR* command, void *var);

CONSOLE_API commands_t* init_commands(int max);
CONSOLE_API void free_commands(commands_t* list);

CONSOLE_API int register_command(commands_t* list, TCHAR* text, TCHAR *help, void (*function)(TCHAR* text, void* var));

CONSOLE_API TCHAR* get_last_error();
