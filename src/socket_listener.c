#include "socket_listener.h"
#include "macrothreading_condition.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN32
typedef int socklen_t;
#include <WinSock2.h>
extern bool winsock_initialized;
extern WSADATA wsaData;
#define SHUT_WR SD_SEND
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif // WIN32

void socket_listener_thread(void *arg)
{
    SOCKET newsockfd;
    socket_listener_t handle = (socket_listener_t)arg;

#ifdef WIN32
    if (listen(handle->sockfd, handle->queue) != 0) {
        handle->error = true;
        return;
    }
#endif // !
    while(!handle->cancellation) {
        
#ifndef WIN32
        if (listen(handle->sockfd, handle->queue) != 0) {
            handle->error = true;
            return;
        }
#endif // !

        /* Accept actual connection from the client */
        newsockfd = accept(handle->sockfd, NULL, NULL);

        if(!handle->cancellation) {            
#ifdef WIN32
            if (newsockfd == INVALID_SOCKET) {
                switch (WSAGetLastError())
                {
                case WSANOTINITIALISED:
                    perror("WSA Not initalized");
                    break;
                case WSAECONNRESET:
                    perror("Connection reset");
                    break;
                case WSAEFAULT:
                    perror("Invalid address");
                    break;
                case WSAEINTR:
                    perror("Blocking conflict");
                    break;
                case WSAEINVAL:
                    perror("Listen function not called");
                    break;
                default:
                    break;
                }
                handle->error = true;
                return;
            }
#else
            if (newsockfd < 0) {
                handle->error = true;
                return;
            }
#endif // WIN32
            
            socket_session_t result = socket_session_init(newsockfd);
            result->state = SOCKET_STATE_CONNECTED;

            // Call the callback
            handle->connection_callback(result, handle->context);
        } else {
            // 
            shutdown(newsockfd, SHUT_WR);
            int recv_result = 0;
            do
            {
                #ifdef WIN32
                recv_result = recv(newsockfd, NULL, 1, 0);
                #else
                recv_result = read(newsockfd, NULL, 1);
                #endif
            } while (recv_result > 0);
            #ifdef WIN32
            closesocket(newsockfd);
            #else
            close(newsockfd);
            #endif
        }
    }
}

socket_listener_t socket_listener_start(int port, int queue, socket_listener_callback callback, void *context)
{
    struct sockaddr_in serv_addr;

    // Create the handle
    socket_listener_t handle = (socket_listener_t)malloc(sizeof(struct socket_listener_struct));
    handle->port = port;
    handle->queue = queue;
    handle->connection_callback = callback;
    handle->thread = NULL;
    handle->cancellation = false;
    handle->context = context;
    handle->error = false;
    handle->started = false;
   
    /* First call to socket() function */
    handle->sockfd = __init_socket();
    if(handle->sockfd == INVALID_SOCKET) {
        handle->error = true;
        return handle;
    }

    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((unsigned short)port);

    /* Allow the listening port to be reusable */
    {
        int option = 1;
        setsockopt(handle->sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option));
    }

    /* Now bind the host address using bind() call.*/
    if (bind(handle->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        handle->error = true;
        return handle;
    }

    // Start the thread
    handle->thread = macrothread_handle_init();
    macrothread_start_thread(handle->thread, socket_listener_thread, handle);
    handle->started = true;
        
    return handle;
}

void socket_listener_stop(socket_listener_t listener)
{
    if(listener == NULL) {
        perror("Socket listener handle is NULL");
        return;
    }

    if(!listener->error && listener->started) {

        // Set the cancellation flag
        listener->cancellation = true;

        // Create the cancel session
        socket_session_t cancel_session = socket_session_create();

        // Create the condition variable
        listener->shutdown_signal = macrothread_condition_init();

        // Connect to the listener
        socket_session_connect(cancel_session, "127.0.0.1", listener->port);

        // Join the canceller
        socket_session_disconnect(cancel_session);

    }

    if(listener->started) {
        macrothread_join(listener->thread);
    }

    // Close the listener socket
    if(listener->sockfd != INVALID_SOCKET) {
        #ifdef WIN32
        closesocket(listener->sockfd);
        #else
        close(listener->sockfd);
        #endif
    }

    // Destroy the thread handle
    if(listener->thread != NULL) {
        macrothread_handle_destroy(listener->thread);
    }

    // Destroy the listener
    free(listener);
}