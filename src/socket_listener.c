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
        perror("ERROR on listen");
        exit(1);
    }
#endif // !
    while(!handle->cancellation) {
        
#ifdef WIN32

#else
        if (listen(handle->sockfd, handle->queue) != 0) {
            perror("ERROR on listen");
            exit(1);
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
                exit(1);
            }
#else
            if (newsockfd < 0) {
                perror("ERROR on accept");
                exit(1);
            }
#endif // WIN32
            
            socket_session_t result = socket_session_init(newsockfd, 1024);
            result->socket->state = SOCKET_STATE_CONNECTED;

            // Call the callback
            handle->connection_callback(result, handle->context);

            // Start monitoring
            socket_session_start(result);
        } else {
            // 
            shutdown(newsockfd, SHUT_WR);
            ssize_t recv_result = 0;
            do
            {
                #ifdef WIN32
                recv(newsockfd, NULL, 1, 0);
                #else
                read(newsockfd, NULL, 1);
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
    handle->cancellation = false;
    handle->context = context;
   
    /* First call to socket() function */
    handle->sockfd = __init_socket();

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
        perror("ERROR on binding");
        exit(1);
    }

    // Start the thread
    handle->thread = macrothread_handle_init();
    macrothread_start_thread(handle->thread, socket_listener_thread, handle);
        
    return handle;
}

void cancel_callback(socket_wrapper_t wrapper)
{
    macrothread_condition_signal((macrothread_condition_t)wrapper->context);
}

void socket_listener_stop(socket_listener_t listener)
{
    if(listener == NULL) {
        perror("Socket listener handle is NULL");
        return;
    }

    // Set the cancellation flag
    listener->cancellation = true;

    // Create the cancel session
    socket_session_t cancel_session = socket_session_create(1);

    // Create the condition variable
    macrothread_condition_t cancel_condition = macrothread_condition_init();

    // Set the context
    cancel_session->socket->context = (void*)cancel_condition;

    // Set the callback
    cancel_session->finalize_callback = cancel_callback;

    // Connect to the listener
    socket_session_connect(cancel_session, "127.0.0.1", listener->port);

    // Join the canceller
    socket_session_disconnect(cancel_session);

    // Wait for shutdown
    macrothread_condition_wait(cancel_condition);

    // Close the listener socket
    #ifdef WIN32
    closesocket(listener->sockfd);
    #else
    close(listener->sockfd);
    #endif

    // Destroy the thread handle
    macrothread_handle_destroy(listener->thread);

    // Destroy the listener
    free(listener);
}