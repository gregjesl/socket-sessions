#ifndef SOCKET_SESSIONS_SOCKET_WRAPPER_H
#define SOCKET_SESSIONS_SOCKET_WRAPPER_H

#include <stdlib.h>
#include <stdio.h>
#include "stdbool.h"
#include "socket_buffer.h"

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
    socket_buffer_t buffer;
    bool connected;
    bool closure_requested;
    void *context;
} *socket_wrapper_t;

enum socket_action_result
{
    SOCKET_ACTION_COMPLETE = 0,
    SOCKET_SESSION_CLOSED = -1,
    SOCKET_SESSION_ERROR = -2
};

socket_wrapper_t socket_wrapper_init(SOCKET seed);
ssize_t socket_wrapper_read(socket_wrapper_t wrapper, char *buffer, size_t max_bytes);
ssize_t socket_wrapper_buffer(socket_wrapper_t wrapper);
int socket_wrapper_write(socket_wrapper_t wrapper, const char *buffer, size_t length);
void socket_wrapper_destroy(socket_wrapper_t wrapper);

#endif