#ifndef SOCKET_SESSIONS_SOCKET_SESSION_H
#define SOCKET_SESSIONS_SOCKET_SESSION_H

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif // !WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
void winsock_init();
#else
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif // WIN32

#include "macrothreading_thread.h"
#include "macrothreading_mutex.h"
#include "macrothreading_condition.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "wolfssl/ssl.h"

typedef enum socket_session_state_enum
{
	SOCKET_STATE_CLOSED,
	SOCKET_STATE_CONNECTED,
	SOCKET_STATE_PEER_CLOSED,
	SOCKET_STATE_SHUTDOWN,
	SOCKET_STATE_ERROR
} socket_session_state_t;

typedef struct socket_session_tls_struct
{
	WOLFSSL* ssl;
	WOLFSSL_CTX* ctx;
} *socket_session_tls_t;

typedef struct socket_session_struct
{
	SOCKET id;
    macrothread_handle_t thread;
	macrothread_mutex_t mutex;
	macrothread_condition_t condition;
	socket_session_state_t state;
	char *buffer;
	size_t buffer_len;
	socket_session_tls_t tls;
} *socket_session_t;

typedef char socket_session_ipv4_address[16];

SOCKET __init_socket();
/* \brief Creates a session from an existing socket 
This funciton is used when a socket already exists and a session needs to be built for the socket
@param id The socket identifier
\returns A socket session
*/
socket_session_t socket_session_init(SOCKET id);
socket_session_t socket_session_create();
size_t socket_session_resolve_ipv4(const char *fqdn, socket_session_ipv4_address result, size_t skip);
void socket_session_enable_tls_client(socket_session_t session, const char *ca_cert);
bool socket_session_tls_enabled(socket_session_t session);
socket_session_state_t socket_session_connect(socket_session_t session, const socket_session_ipv4_address, const int port);
size_t socket_session_read(socket_session_t session, char *buffer, size_t max_bytes);
size_t socket_session_write(socket_session_t session, const char *buffer, size_t bytes_to_write);
socket_session_state_t socket_session_wait(socket_session_t session);
size_t socket_session_buffer(socket_session_t session, size_t min_bytes, size_t max_bytes);
typedef bool(socket_buffer_condition)(const char *, size_t);
bool socket_session_buffer_until(socket_session_t session, socket_buffer_condition condition, size_t increment, size_t max_bytes);
size_t socket_session_flush(socket_session_t session, size_t bytes_to_flush);
void socket_session_shutdown(socket_session_t session);
void socket_session_disconnect(socket_session_t session);
void socket_session_destroy(socket_session_t session);
void socket_session_cleanup();

#endif