#ifndef BIRCHRIDGE_SOCKET_DATA_H
#define BIRCHRIDGE_SOCKET_DATA_H

#include <stdlib.h>
#include "socket_error.h"

typedef struct socket_data_struct
{
    char *buffer;
    char *write_buffer;
    size_t buffer_length;
} *socket_data_t;

socket_data_t socket_data_init(size_t max_buffer_length);
size_t socket_data_length(socket_data_t data);
int socket_data_push(socket_data_t data, const char *buffer, size_t length);
int socket_data_pop(socket_data_t data, size_t length);
void socket_data_destroy(socket_data_t data);

#endif