#include "socket_session.h"
#include "socket_manager.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <WinSock2.h>
#define poll(a,b,c) WSAPoll(a, b, c)
static bool winsock_initialized = false;
static WSADATA wsaData = { 0 };
#else
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#endif // WIN32

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
    if ((sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
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

socket_session_t socket_session_connect(const char *address, const int port)
{
    SOCKET sock = 0; 
    struct sockaddr_in serv_addr;

   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);

    sock = __init_socket();
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0)  
    { 
        perror("Invalid address/ Address not supported"); 
        return NULL;
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        perror("Connection Failed"); 
        return NULL;
    }

    // Create the session
    socket_session_t result = (socket_session_t)malloc(sizeof(struct socket_session_struct));
    result->socket = socket_wrapper_init(sock);
    result->data_ready_callback = NULL;
    result->closure_callback = NULL;
    result->monitor = false;
    result->thread = NULL;
    return result;
}

int socket_session_write(socket_session_t session, const char *data, const size_t length)
{
    return socket_wrapper_write(session->socket, data, length);
}

void monitor_thread(void *arg)
{
    struct pollfd pfd;
    int result;
    socket_session_t monitor = (socket_session_t)arg;
    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = monitor->socket->id;
    pfd.events = POLLIN;
    while(monitor->monitor) {
        result = poll(&pfd, 1, 10);
        if(result > 0) {
            if(pfd.revents & POLLIN && monitor->data_ready_callback != NULL) {
                monitor->data_ready_callback(monitor->socket);
                if(monitor->socket->closure_requested) {
                    shutdown(monitor->socket->id, SHUT_WR);
                    ssize_t recv_result = 0;
                    do
                    {
                        recv_result = read(monitor->socket->id, NULL, 1);
                    } while (recv_result > 0);
                    socket_session_close(monitor);
                    break;
                }
            }

            if(pfd.revents & POLLHUP) {
                monitor->monitor = false;
                socket_session_close(monitor);
            }

            if(pfd.revents & POLLNVAL) {
                perror("Polled socket that is closed");
                monitor->monitor = false;
            }

            if(pfd.revents & POLLERR) {
                perror("Poll error");
                monitor->monitor = false;
            }
        } else if(result == 0) {
            continue;
        } else {
            perror("Error encountered");
            monitor->monitor = false;
        }
    }
}

void socket_session_start(socket_session_t session, socket_session_callback data_ready, socket_session_callback closure_callback)
{
    if(session->thread != NULL) {
        perror("Socket session already running");
        return;
    }
    session->data_ready_callback = data_ready;
    session->closure_callback = closure_callback;
    session->monitor = true;
    session->thread = macrothread_handle_init();
    macrothread_start_thread(session->thread, monitor_thread, session);
}

void socket_session_stop(socket_session_t monitor)
{
    if(monitor->thread == NULL)
        return;
    
    monitor->monitor = false;
    macrothread_join(monitor->thread);
    macrothread_handle_destroy(monitor->thread);
    monitor->thread = NULL;
}

void socket_manager_thread(void *arg)
{
    socket_manager_t manager = (socket_manager_t)arg;

    // Copy the session
    socket_session_t session = (socket_session_t)manager->incoming_session;

    // Signal the copy
    macrothread_condition_signal(manager->confirmation);

    // Stop monitoring
    socket_session_stop(session);

    // Destroy the wrapper
    socket_wrapper_destroy(session->socket);

    // Free the memory
    free(session);
}

void socket_session_finalize(socket_manager_t manager, socket_session_t session)
{
    // Lock the manager
    macrothread_mutex_lock(manager->mutex);

    // Wait until the last thread finishes
    macrothread_join(manager->handle);

    // Set the session
    manager->incoming_session = session;

    // Start the thread
    macrothread_start_thread(manager->handle, socket_manager_thread, manager);

    // Wait for the confirmation
    macrothread_condition_wait(manager->confirmation);

    // Unlock the mutex
    macrothread_mutex_unlock(manager->mutex);
}

void socket_session_close(socket_session_t session)
{
    static socket_manager_t garbage_collector = NULL;
    #ifdef WIN32
    closesocket(session->socket->id);
    #else
    close(session->socket->id);
    #endif

    if(session->closure_callback != NULL) {
        session->closure_callback(session->socket);
    }

    if(garbage_collector == NULL) {
        garbage_collector = socket_manager_init();
    }

    socket_session_finalize(garbage_collector, session);
}
