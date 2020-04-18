#include "socket_session.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

socket_session_t socket_session_init()
{
    socket_session_t result = (socket_session_t)malloc(sizeof(struct socket_session_struct));
    result->socket = 0;
    result->buffer = buffer_chain_init(1024);
    result->connected = false;
    result->context = NULL;
    return result;
}

int socket_session_read(socket_session_t session)
{
    struct pollfd pfd;
    int pollreturn;
    char buffer[1024];
    ssize_t bytes_read;
    size_t total_bytes_read = 0;

    if(!session->connected) 
        return SOCKET_SESSION_CLOSED;

    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = session->socket;
    pfd.events = POLLIN;

    while(poll(&pfd, 1, 0) > 0)
    {
        if(pfd.revents & POLLIN) {
            #ifdef WIN32
            bytes_read = recv(session->socket, buffer, 1024, 0);
            #else
            bytes_read = read(session->socket, buffer, 1024);
            #endif

            if(bytes_read > 0) {
                buffer_chain_write(session->buffer, buffer, bytes_read);
                total_bytes_read += bytes_read;
            }
        }

        if(pfd.revents & POLLHUP) {
            socket_session_close(session);
            break;
        }

        if(pfd.revents & POLLERR) {
            perror("Poll error");
        }
    }
    
    return total_bytes_read > 0 ? SOCKET_SESSION_DATA_READ : SOCKET_SESSION_NO_CHANGE;
}

int socket_session_write(socket_session_t session, const char *data, const size_t length)
{
    ssize_t bytes_written = 0;
    const char *write_index = data;
    size_t bytes_to_write = length;

    while(bytes_to_write > 0) {
        #ifdef WIN32
        bytes_written = send(session->socket, write_index, bytes_to_write, 0);
        #else
        bytes_written = write(session->socket, write_index, bytes_to_write);
        #endif

        if(bytes_written < 0) {
            socket_session_close(session);
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

void monitor_thread(void *arg)
{
    struct pollfd pfd;
    int result;
    socket_monitor_t monitor = (socket_monitor_t)arg;
    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = monitor->session->socket;
    pfd.events = POLLIN;
    while(monitor->monitor) {
        result = poll(&pfd, 1, 10);
        if(result > 0) {
            if(pfd.revents & POLLIN) {
                if(socket_session_read(monitor->session) == SOCKET_SESSION_DATA_READ
                && monitor->data_ready_callback != NULL) {
                    monitor->data_ready_callback(monitor->session);
                }
            }

            if(pfd.revents & POLLHUP) {
                socket_session_close(monitor->session);
                if(monitor->closure_callback != NULL) {
                    monitor->closure_callback(monitor->session);
                }
                monitor->monitor = false;
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

socket_monitor_t socket_session_monitor(socket_session_t session, socket_session_callback data_ready, socket_session_callback closure_callback)
{
    socket_monitor_t result = (socket_monitor_t)malloc(sizeof(struct socket_monitor_struct));
    result->session = session;
    result->data_ready_callback = data_ready;
    result->closure_callback = closure_callback;
    result->monitor = true;
    result->thread = macrothread_handle_init();
    macrothread_start_thread(result->thread, monitor_thread, result);
    return result;
}

void socket_session_stop_monitor(socket_monitor_t monitor)
{
    if(monitor->thread == NULL) {
        perror("Null arguement");
        exit(1);
    }
    monitor->monitor = false;
    macrothread_join(monitor->thread);
    macrothread_handle_destroy(monitor->thread);
    monitor->thread = NULL;
    free(monitor);
}

void socket_session_close(socket_session_t session)
{
    #ifdef WIN32
    closesocket(session->socket);
    WSACleanup();
    #else
    close(session->socket);
    #endif
    session->connected = false;
}

void socket_session_destroy(socket_session_t session)
{
    buffer_chain_destroy(session->buffer);
    free(session);
}

void socket_session_connect(socket_session_t session, const char *address, const int port)
{
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        perror("Socket creation error"); 
        exit(1);
    }
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0)  
    { 
        perror("Invalid address/ Address not supported"); 
        exit(1);
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        perror("Connection Failed"); 
        exit(1);
    }

    session->connected = true;
    session->socket = sock;
}