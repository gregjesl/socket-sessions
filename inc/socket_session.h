#ifndef SOCKET_SESSIONS_SOCKET_SESSION_H
#define SOCKET_SESSIONS_SOCKET_SESSION_H

#include "macrothreading_thread.h"

#include "buffer_chain.h"
#include <stdbool.h>
#ifdef WIN32
#include <Winsock.h>
#else
#include <unistd.h>
#include <errno.h>
typedef int SOCKET;
#endif

typedef struct socket_session_struct
{
    SOCKET socket;
    buffer_chain_t buffer;
    bool connected;
    void *context;
    
} *socket_session_t;

typedef void (socket_session_callback)(socket_session_t);

enum socket_action_result
{
    SOCKET_SESSION_DATA_READ,
    SOCKET_SESSION_DATA_WRITTEN,
    SOCKET_SESSION_NO_CHANGE,
    SOCKET_SESSION_CLOSED,
    SOCKET_SESSION_ERROR
};

typedef struct socket_monitor_struct
{
    socket_session_t session;
    socket_session_callback *data_ready_callback;
    socket_session_callback *closure_callback;
    bool monitor;
    macrothread_handle_t thread;
} *socket_monitor_t;

socket_session_t socket_session_init();
int socket_session_read(socket_session_t session);
int socket_session_write(socket_session_t session, const char *data, const size_t length);
socket_monitor_t socket_session_monitor(socket_session_t session, socket_session_callback data_ready, socket_session_callback closure_callback);
void socket_session_stop_monitor(socket_monitor_t monitor);
void socket_session_close(socket_session_t session);
void socket_session_destroy(socket_session_t session);

void socket_session_connect(socket_session_t session, const char *address, const int port);

#endif