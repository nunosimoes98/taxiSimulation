// Console.cpp : Defines the exported functions for the DLL.
//

#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <strsafe.h>
#include <errno.h>

#include "pch.h"
#include "framework.h"
#include "Console.h"


CONSOLE_API void read_text(TCHAR* text, TCHAR* output, int size)
{
    for (;;) {
        _tprintf(L"%s", text);
        _fgetts(output, size, stdin);
        output[_tcslen(output) - 1] = '\0'; // Delete last char (\n)
        if (_tcslen(output) != 0) {
            return;
        }
    }
}

CONSOLE_API TCHAR* read_command()
{
    static TCHAR text[CONSOLE_MAX_TEXT]; // Keep variable in memory, even after return
    read_text(L"> ", text, CONSOLE_MAX_TEXT);
    return text;
}

// Function to read a integer from the console, give a text to show before hand
CONSOLE_API int read_number(TCHAR* text)
{
    int number;
    TCHAR buffer[CONSOLE_MAX_TEXT];

    for (;;) {
        read_text(text, buffer, CONSOLE_MAX_TEXT);
        int valid = 1;
        for (size_t i = 0; i < _tcslen(buffer); i++) {
            if (buffer[i] < '0' || buffer[i] > '9') {
                valid = 0;
                break;
            }
        }
        if (valid) {
            number = _tstoi(buffer);
            return number;
        }

    }
}


void show_help(TCHAR* text, void* arg)
{
    _CRT_UNUSED(text);

    commands_t* list = (commands_t*)arg;
    _tprintf(L"\nList of available commands:\n");
    for (int i = 0; i < list->count; i++) {
        _tprintf(L"%s - %s\n", list->list[i].name, list->list[i].help);
    }

}

CONSOLE_API int execute_command(commands_t* list, TCHAR* full_command, void* var)
{
    // TODO remove return int and from all 
    TCHAR *command_args = NULL;
    TCHAR *command = _tcstok_s(full_command, L" ", &command_args);

    char executed = CONSOLE_NOT_EXECUTED;
    if (_tcsicmp(command, L"help") == 0) {
        executed = CONSOLE_EXECUTED;
        show_help(NULL, list);
    }

    if (!executed) {
        for (int i = 0; i < list->count; i++) {
            if (_tcsicmp(list->list[i].name, command) == 0) {
                list->list[i].function(command_args, var);
                executed = CONSOLE_EXECUTED;
                break;
            }
        }
    }
    if (!executed) {
        _tprintf(L"Invalid command\n");
        show_help(NULL, list);
    }
    return executed;
}

CONSOLE_API commands_t* init_commands(int max)
{
    commands_t* list = (commands_t*) malloc(sizeof(commands_t));
    if (list == NULL) {
        return NULL;
    }
    list->list = (command_t*) malloc(sizeof(command_t) * max);
    if (list->list == NULL) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->max = max;

    register_command(list, L"help", L"Shows list of available command", show_help);
    return list;
}

CONSOLE_API void free_commands(commands_t* list)
{
    if (list != NULL) {
        free(list->list);
        free(list);
    }
}



CONSOLE_API int register_command(commands_t* list, TCHAR* text, TCHAR* help, void (*function)(TCHAR* text, void *var))
{
    if (list->count == list->max) {
        // No more commands can be registered
        return CONSOLE_ERROR;
    }

    // TODO, if there are commands with the same name

    _tcscpy_s(list->list[list->count].name, CONSOLE_MAX_TEXT, text);
    _tcscpy_s(list->list[list->count].help, CONSOLE_MAX_TEXT_HELP, help);
    list->list[list->count].function = function;
    list->count++;

    return CONSOLE_NOERROR;
}

CONSOLE_API TCHAR* get_last_error()
{
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR*)&lpMsgBuf, 0, NULL);

    return (TCHAR*)lpMsgBuf;
}