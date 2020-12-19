#include "socket_session.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#define poll(a,b,c) WSAPoll(a, b, c)
static bool winsock_initialized = false;
static WSADATA wsaData = { 0 };
#define inet_pton(a, b, c) InetPton(a,b,c)
#else
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#endif // WIN32

#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)

SOCKET __init_socket()
{
#ifdef WIN32
    SOCKET sock = INVALID_SOCKET;
    if (!winsock_initialized) {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            perror("Could not initialize WSA");
            exit(1);
        }
        winsock_initialized = true;
    }
    if ((sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        perror("Socket creation error");
    }
#else
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation error");
    }
#endif // WIN32
    return sock;
}

ssize_t __read_cycle(socket_session_t session, int poll_period_ms)
{
    // Attempt to write data to the buffer
    const ssize_t result = socket_wrapper_read(
        session->socket,
        session->socket->data->write_buffer,
        session->socket->data->buffer_length - socket_data_length(session->socket->data),
        poll_period_ms
    );

    // Handle new data
    if(result > 0) {

        // Move the write index
        session->socket->data->write_buffer += result;

        // Call the data callback
        if(session->data_callback != NULL) {
            session->data_callback(session->socket);
        }
    }

    return result;
}

void socket_wrapper_close(socket_wrapper_t wrapper)
{
    #ifdef WIN32
    closesocket(wrapper->id);
    #else
    close(wrapper->id);
    #endif
    wrapper->state = SOCKET_STATE_CLOSED;
}

void socket_wrapper_destroy(socket_wrapper_t wrapper)
{
    socket_data_destroy(wrapper->data);
    free(wrapper);
}

void monitor_thread(void *arg)
{
    const int poll_period_ms = 10;
    socket_session_t session = (socket_session_t)arg;

    do
    {
        __read_cycle(session, poll_period_ms);

        // Check for a timeout
        if(
            session->socket->timeout > 0.0f
            &&
            session->socket->state == SOCKET_STATE_CONNECTED
            &&
            (float)session->socket->last_activity / 1000.0f > session->socket->timeout
        ) {
            session->socket->state = SOCKET_STATE_TIMEOUT;
        }

    } while (session->socket->state == SOCKET_STATE_CONNECTED);

    switch (session->socket->state)
    {
    case SOCKET_STATE_CLOSED:
    case SOCKET_STATE_PEER_CLOSED:
        while(session->socket->state == SOCKET_STATE_PEER_CLOSED) {
            __read_cycle(session, 0);
        }
        if(session->hangup_callback != NULL) {
            session->hangup_callback(session->socket);
        }
        break;
    case SOCKET_STATE_TIMEOUT:
        if(session->timeout_callback != NULL) {
            session->timeout_callback(session->socket);
        }
        break;
    case SOCKET_STATE_SHUTDOWN:
        // Do nothing
        break;
    default:
    case SOCKET_STATE_ERROR:
        session->socket->state = SOCKET_STATE_ERROR;
        if(session->error_callback != NULL) {
            session->error_callback(session->socket);
        }
        break;
    }

    // Close the socket
    socket_wrapper_close(session->socket);
    assert(session->socket->state == SOCKET_STATE_CLOSED);

    // Notify 
    if(session->finalize_callback != NULL) {
        session->finalize_callback(session->socket);
    }

    // Wait for the signal to shut down
    socket_wrapper_destroy(session->socket);
    free(session);
}

socket_session_t socket_session_init(SOCKET id, size_t max_buffer)
{
    socket_session_t result = (socket_session_t)malloc(sizeof(struct socket_session_struct));
    result->thread = NULL;
    
    // Initialize the socket
    result->socket = socket_wrapper_init(id, max_buffer);

    // Set all the callbacks
    result->data_callback = NULL;
    result->hangup_callback = NULL;
    result->timeout_callback = NULL;
    result->error_callback = NULL;
    result->finalize_callback = NULL;

    // Return the result
    return result;
}

socket_session_t socket_session_create(size_t max_buffer)
{
    return socket_session_init(__init_socket(), max_buffer);
}

int socket_session_connect(socket_session_t session, const char *address, const int port)
{
    struct sockaddr_in serv_addr;

    // Validate the inputs
    if(session == NULL || address == NULL) return SOCKET_ERROR_NULL_ARGUEMENT;
    if(session->thread != NULL) return SOCKET_ERROR_CONFLICT;

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons((unsigned short)port);
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0) return SOCKET_ERROR_INVALID_ADDRESS;
   
    unsigned int retry_count = 0;
    while(retry_count < 3) {
        if (connect(session->socket->id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            if(retry_count > 2) {
                perror("Error encountered in connect()");
                return SOCKET_ERROR_CONNECT;
            } else {
                retry_count++;
                macrothread_delay(10);
            }
        } else {
            break;
        }
    }

    session->socket->state = SOCKET_STATE_CONNECTED;

    // Start the session
    socket_session_start(session);

    return SOCKET_OK;
}

void socket_session_start(socket_session_t session)
{
    // Start monitoring the socket
    session->thread = macrothread_handle_init();
    session->thread->detached = true;
    macrothread_start_thread(session->thread, monitor_thread, session);
}

void socket_session_disconnect(socket_session_t session)
{
    socket_wrapper_shutdown(session->socket);
}