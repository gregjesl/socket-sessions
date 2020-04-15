#ifndef SOCKET_SESSIONS_SOCKET_SESSION_H
#define SOCKET_SESSIONS_SOCKET_SESSION_H

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
} *socket_session_t;

enum socket_action_result
{
    SOCKET_SESSION_DATA_READ,
    SOCKET_SESSION_DATA_WRITTEN,
    SOCKET_SESSION_NO_CHANGE,
    SOCKET_SESSION_CLOSED,
    SOCKET_SESSION_ERROR
};

typedef void (socket_session_callback)(socket_session_t);

socket_session_t socket_session_init(int sock);
int socket_session_read(socket_session_t session);
int socket_session_write(socket_session_t session, const char *data, const size_t length);
void socket_session_close(socket_session_t session);
void socket_session_destroy(socket_session_t session);

void socket_session_listen(int port, socket_session_callback callback, size_t queue, bool *cancellation);
socket_session_t socket_session_connect(const char *address, const int port);

#endif