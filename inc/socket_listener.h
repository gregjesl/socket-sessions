#ifndef SOCKET_SESSIONS_SOCKET_LISTENER_H
#define SOCKET_SESSIONS_SOCKET_LISTENER_H

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif // !WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#endif // WIN32

#include "socket_session.h"
#include "macrothreading_thread.h"

typedef void(socket_listener_callback)(socket_session_t, void*);

typedef struct socket_listener_struct
{
    SOCKET sockfd;
    int port;
    int queue;
    socket_listener_callback *connection_callback;
    macrothread_handle_t thread;
    bool cancellation;
    void *context;
} *socket_listener_t;

socket_listener_t socket_listener_start(int port, int queue, socket_listener_callback callback, void *context);
void socket_listener_stop(socket_listener_t listener);

#endif