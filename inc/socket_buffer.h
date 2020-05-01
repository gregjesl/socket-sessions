#ifndef SOCKET_SESSIONS_SOCKET_BUFFER_H
#define SOCKET_SESSIONS_SOCKET_BUFFER_H

#include <stdio.h>
#include <stdlib.h>

typedef struct socket_buffer_struct
{
    FILE *stream;
    char *data;
    size_t length;
} *socket_buffer_t;

socket_buffer_t socket_buffer_init();
void socket_buffer_write(socket_buffer_t buffer, const char* data, size_t length);
void socket_buffer_splice(socket_buffer_t buffer, size_t index);
void socket_buffer_destroy(socket_buffer_t buffer);

#endif