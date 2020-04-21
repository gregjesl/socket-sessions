#include "socket_session.h"
#include "socket_manager.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

socket_wrapper_t socket_wrapper_init(SOCKET id)
{
    socket_wrapper_t result = (socket_wrapper_t)malloc(sizeof(struct socket_wrapper_struct));
    result->id = id;
    result->buffer = buffer_chain_init(1024);
    result->connected = true;
    result->context = NULL;
    return result;
}

int socket_wrapper_read(socket_wrapper_t session)
{
    struct pollfd pfd;
    int pollreturn;
    char buffer[1024];
    ssize_t bytes_read;
    size_t total_bytes_read = 0;

    if(!session->connected) 
        return SOCKET_SESSION_CLOSED;

    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = session->id;
    pfd.events = POLLIN;

    while(poll(&pfd, 1, 0) > 0)
    {
        if(pfd.revents & POLLIN) {
            #ifdef WIN32
            bytes_read = recv(session->id, buffer, 1024, 0);
            #else
            bytes_read = read(session->id, buffer, 1024);
            #endif

            if(bytes_read > 0) {
                buffer_chain_write(session->buffer, buffer, bytes_read);
                total_bytes_read += bytes_read;
            }
        }

        if(pfd.revents & POLLHUP) {
            return SOCKET_SESSION_CLOSED;
            break;
        }

        if(pfd.revents & POLLERR) {
            perror("Poll error");
        }
    }
    
    return total_bytes_read > 0 ? SOCKET_SESSION_DATA_READ : SOCKET_SESSION_NO_CHANGE;
}

int socket_wrapper_write(socket_wrapper_t session, const char *data, const size_t length)
{
    ssize_t bytes_written = 0;
    const char *write_index = data;
    size_t bytes_to_write = length;

    while(bytes_to_write > 0) {
        #ifdef WIN32
        bytes_written = send(session->id, write_index, bytes_to_write, 0);
        #else
        bytes_written = write(session->id, write_index, bytes_to_write);
        #endif

        if(bytes_written < 0) {
            #ifdef WIN32
            if(bytes_written == WSAETIMEDOUT ||
                bytes_written == WSAECONNRESET ||
            ) {
                return SOCKET_SESSION_CLOSED;
            } else {
                return SOCKET_SESSION_ERROR;
            }
            #else
            return bytes_written == EPIPE ? SOCKET_SESSION_CLOSED : SOCKET_SESSION_ERROR;
            #endif
        }

        bytes_to_write -= bytes_written;
        write_index += bytes_written;
    }

    return length > 0 ? SOCKET_SESSION_DATA_WRITTEN : SOCKET_SESSION_NO_CHANGE;
}

void socket_wrapper_destroy(socket_wrapper_t session)
{
    buffer_chain_destroy(session->buffer);
    free(session);
}

socket_session_t socket_session_connect(const char *address, const int port)
{
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        perror("Socket creation error"); 
        return NULL;
    }
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
       
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

int socket_session_buffer(socket_session_t session)
{
    return socket_wrapper_read(session->socket);
}

void socket_session_write(socket_session_t session, const char *data, const size_t length)
{
    socket_wrapper_write(session->socket, data, length);
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
            if(pfd.revents & POLLIN) {
                if(socket_wrapper_read(monitor->socket) == SOCKET_SESSION_DATA_READ
                && monitor->data_ready_callback != NULL) {
                    monitor->data_ready_callback(monitor->socket);
                }
            }

            if(pfd.revents & POLLHUP) {
                if(monitor->closure_callback != NULL) {
                    monitor->closure_callback(monitor->socket);
                }
                socket_session_close(monitor);
                monitor->monitor = false;
                continue;
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
    WSACleanup();
    #else
    close(session->socket->id);
    #endif

    if(garbage_collector == NULL) {
        garbage_collector = socket_manager_init();
    }

    socket_session_finalize(garbage_collector, session);
}
