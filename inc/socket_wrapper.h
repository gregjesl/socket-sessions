#ifndef SOCKET_SESSIONS_SOCKET_WRAPPER_H
#define SOCKET_SESSIONS_SOCKET_WRAPPER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "socket_data.h"
#include "macrothreading_condition.h"

#ifdef WIN32
typedef int ssize_t;
#include <WinSock2.h>
#else
#include <unistd.h>
#include <errno.h>
typedef int SOCKET;
#endif

enum socket_state_emum
{
    SOCKET_STATE_CLOSED,
    SOCKET_STATE_CONNECTED,
    SOCKET_STATE_PEER_CLOSED,
    SOCKET_STATE_SHUTDOWN,
    SOCKET_STATE_TIMEOUT,
    SOCKET_STATE_ERROR
};

typedef int socket_state_t;

typedef struct socket_wrapper_struct
{
    SOCKET id;
    socket_data_t data;
    socket_state_t state;
    unsigned long last_activity;
    float timeout;
    void *context;
} *socket_wrapper_t;

socket_wrapper_t socket_wrapper_init(SOCKET seed, size_t max_buffer_length);
ssize_t socket_wrapper_read(socket_wrapper_t wrapper, char *buffer, size_t max_bytes, int poll_period_ms);
int socket_wrapper_write(socket_wrapper_t wrapper, const char *buffer, size_t length);
int socket_wrapper_shutdown(socket_wrapper_t wrapper);

#endif