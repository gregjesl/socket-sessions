#ifndef SOCKET_SESSIONS_SOCKET_SESSION_POOL_H
#define SOCKET_SESSIONS_SOCKET_SESSION_POOL_H

#include "socket_session.h"

typedef struct socket_session_pool_struct
{
    socket_session_t *sessions;
    size_t num_sessions;
} *socket_session_pool_t;

socket_session_pool_t socket_session_pool_init();
void socket_session_pool_add(socket_session_pool_t pool, socket_session_t session);
void socket_session_pool_remove(socket_session_pool_t pool, socket_session_t session);
void socket_session_pool_clear(socket_session_pool_t pool);
void socket_session_pool_destroy(socket_session_pool_t pool);
socket_session_pool_t socket_session_pool_poll(socket_session_pool_t pool);

#endif