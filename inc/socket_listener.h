#ifndef SOCKET_SESSIONS_SOCKET_LISTENER_H
#define SOCKET_SESSIONS_SOCKET_LISTENER_H

#include "socket_session.h"
#include "macrothreading_thread.h"

typedef void(socket_listener_callback)(socket_session_t);

typedef struct socket_listener_struct
{
    SOCKET sockfd;
    int port;
    int queue;
    socket_listener_callback *connection_callback;
    macrothread_handle_t thread;
    bool cancellation;
} *socket_listener_t;

socket_listener_t socket_listener_start(int port, int queue, socket_listener_callback callback);
void socket_listener_stop(socket_listener_t listener);

#endif