#ifndef SOCKET_SESSIONS_SOCKET_SESSION_H
#define SOCKET_SESSIONS_SOCKET_SESSION_H

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif // !WIN32_LEAN_AND_MEAN
#endif // WIN32

#include "macrothreading_thread.h"
#include "socket_wrapper.h"

#include <stdio.h>
#include <stdbool.h>

typedef void (socket_session_callback)(socket_wrapper_t);

typedef struct socket_session_struct
{
    macrothread_handle_t thread;
    socket_wrapper_t socket;
    socket_session_callback *data_callback;
    socket_session_callback *timeout_callback;
    socket_session_callback *hangup_callback;
    socket_session_callback *error_callback;
    socket_session_callback *finalize_callback;
} *socket_session_t;

SOCKET __init_socket();
socket_session_t socket_session_init(SOCKET id, size_t max_buffer);
socket_session_t socket_session_create(size_t max_buffer);
int socket_session_connect(socket_session_t session, const char *address, const int port);
void socket_session_start(socket_session_t session, bool detach);
void socket_session_disconnect(socket_session_t session);

#endif