#ifndef SOCKET_SESSIONS_SOCKET_SESSION_H
#define SOCKET_SESSIONS_SOCKET_SESSION_H

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif // !WIN32_LEAN_AND_MEAN
#endif // WIN32

#include "macrothreading_thread.h"
#include "socket_manager.h"
#include "socket_wrapper.h"
#include "socket_buffer.h"

#include <stdio.h>
#include <stdbool.h>

typedef void (socket_session_callback)(socket_wrapper_t);

typedef struct socket_session_struct
{
    socket_wrapper_t socket;
    socket_session_callback *data_ready_callback;
    socket_session_callback *closure_callback;
    bool monitor;
    macrothread_handle_t thread;
} *socket_session_t;

SOCKET __init_socket();
socket_session_t socket_session_connect(const char *address, const int port);
int socket_session_write(socket_session_t session, const char *data, const size_t length);
void socket_session_start(socket_session_t session, socket_session_callback data_ready, socket_session_callback closure_callback);
void socket_session_stop(socket_session_t session);
void socket_session_close(socket_session_t session);

#endif