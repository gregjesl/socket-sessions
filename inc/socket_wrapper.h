#ifndef SOCKET_SESSIONS_SOCKET_WRAPPER_H
#define SOCKET_SESSIONS_SOCKET_WRAPPER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "socket_data.h"

#ifdef WIN32
typedef int ssize_t;
#include <WinSock2.h>
#else
#include <unistd.h>
#include <errno.h>
typedef int SOCKET;
#endif

typedef struct socket_wrapper_struct
{
    SOCKET id;
    short state;
    socket_data_t data;
    bool connected;
    unsigned long last_activity;
    float timeout;
    void *context;
} *socket_wrapper_t;

socket_wrapper_t socket_wrapper_init(SOCKET seed, size_t max_buffer_length);
ssize_t socket_wrapper_buffer(socket_wrapper_t wrapper, int poll_period_ms);
int socket_wrapper_write(socket_wrapper_t wrapper, const char *buffer, size_t length);
int socket_wrapper_close(socket_wrapper_t wrapper);
void socket_wrapper_destroy(socket_wrapper_t wrapper);

#endif