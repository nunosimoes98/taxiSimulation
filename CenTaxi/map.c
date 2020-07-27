#include <wtypes.h>
#include <stdio.h>
#include <tchar.h>

#include "data.h"
#include "constants.h"
#include "../Common/common.h"
#include "../ConTaxiIPC/contaxi_ipc.h"



int validate_map_content(char* content, int* lines, int* columns, int full_size)
{
    // check size of first line
    char* next_token = NULL;
    char* token = strtok_s(content, "\n", &next_token);
    *lines = 1;
    *columns = (int)strlen(token);
    if (*columns < MIN_SIZE_MAP) {
        return TAXI_ERROR;
    }

    // It can't have only one line
    if (*columns == full_size) {
        return TAXI_ERROR;
    }
    token[*columns] = '\n'; // re add the \n in the original content

    // Check if all lines have the same length
    while (next_token) {
        token = strtok_s(NULL, "\n", &next_token);
        if (token == NULL) {
            break;
        }
        (*lines)++;
        int length = (int)strlen(token);
        if (length != *columns) {
            // different size
            return TAXI_ERROR;
        }
        if ((*columns + 1) * (*lines) < full_size) {
            token[*columns] = '\n'; // re add the \n in the original content
        }
    }

    if (*lines < MIN_SIZE_MAP) {
        return TAXI_ERROR;
    }

    return TAXI_NOERROR;
}

/**
 * Loads the line content in the map inside the settings variable
 */
int load_line(map_t* map, char* content, int num_line)
{
    for (int i = 0; i < *map->width; i++) {
        node_t* n = get_node(map, i, num_line);
        if (content[i] == BLOCK || content[i] == ROAD) {
            n->type = content[i]; // update type
            // Update coordinates
            n->node.x = i;
            n->node.y = num_line;
        }
        else {
            return TAXI_ERROR;
        }
    }
    return TAXI_NOERROR;
}

/**
 * Allocates shared memory to store map information
 */
int init_map_ipc_resources(int lines, int columns, map_t* map)
{
    map->shm_size = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int) * 2, SHM_MAP_SIZE);
    if (map->shm_size == NULL) {
        return TAXI_ERROR;
    }
    map->height = (int*)MapViewOfFile(map->shm_size, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(int) * 2);
    if (map->height == NULL) {
        CloseHandle(map->shm_size);
        return TAXI_ERROR;
    }
    map->width = &map->height[1];
    *map->height = lines;
    *map->width = columns;

    int nodes_size = (*map->height) * (*map->width) * sizeof(node_t);

    map->shm_nodes = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, nodes_size, SHM_MAP);
    if (map->shm_nodes == NULL) {
        CloseHandle(map->shm_size);
        UnmapViewOfFile(map->shm_size);
        return TAXI_ERROR;
    }

    map->nodes = (node_t*)MapViewOfFile(map->shm_nodes, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, nodes_size);
    if (map->nodes == NULL) {
        CloseHandle(map->shm_nodes);
        CloseHandle(map->shm_size);
        UnmapViewOfFile(map->shm_size);
        return TAXI_ERROR;
    }

    return TAXI_NOERROR;
}

/**
 * Loads the map from the file and store it in the settings
 * variable. File must be in UTF-8.
 */
int load_map(settings_t* settings)
{
    // Open file
    FILE* fp;
    errno_t err = _tfopen_s(&fp, settings->map_filename, TEXT("r"));
    if (err != 0) {
        return TAXI_ERROR;
    }

    // Read the the whole file to memory - TODO -> move to function
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    char* content = malloc(sizeof(char) * (size + 1));
    if (content == NULL) {
        fclose(fp);
        return TAXI_ERROR;
    }
    fseek(fp, 0, SEEK_SET); // set the pointer at the begging of the file
    fread(content, sizeof(char), size, fp);
    content[size] = '\0';
    fclose(fp);

    int lines, columns;
    if (!validate_map_content(content, &lines, &columns, size)) {
        free(content);
        return TAXI_ERROR;
    }

    if (!init_map_ipc_resources(lines, columns, &settings->map)) {
        free(content);
        return TAXI_ERROR;
    }
     
    // Load data from content in the map
    char* next_token = NULL;
    char* token = strtok_s(content, "\n", &next_token);
    if (!load_line(&settings->map, content, 0)) {
        free(content);
        free(settings->map.nodes);
        return TAXI_ERROR;
    }
    int line = 1;
    while (token) {
        token = strtok_s(NULL, "\n", &next_token);
        if (token == NULL) {
            break;
        }
        if (!load_line(&settings->map, token, line)) {
            free(content);
            free(settings->map.nodes);
            return TAXI_ERROR;
        }
        line++;
    }

    free(content);

    return TAXI_NOERROR;
}