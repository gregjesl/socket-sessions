#include "socket_listener.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN32
typedef int socklen_t;
#include <WinSock2.h>
extern bool winsock_initialized;
extern WSADATA wsaData;
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


        if(!handle->cancellation) {
   
            /* Accept actual connection from the client */
            newsockfd = accept(handle->sockfd, NULL, NULL);
            
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
            
            socket_session_t result = (socket_session_t)malloc(sizeof(struct socket_session_struct));
            result->socket = socket_wrapper_init(newsockfd);
            result->data_ready_callback = NULL;
            result->closure_callback = NULL;
            result->monitor = false;
            result->thread = NULL;

            // Call the callback
            handle->connection_callback(result);
        }
    }
}

socket_listener_t socket_listener_start(int port, int queue, socket_listener_callback callback)
{
    struct sockaddr_in serv_addr;

    // Create the handle
    socket_listener_t handle = (socket_listener_t)malloc(sizeof(struct socket_listener_struct));
    handle->port = port;
    handle->queue = queue;
    handle->connection_callback = callback;
    handle->cancellation = false;
   
    /* First call to socket() function */
    handle->sockfd = __init_socket();

    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

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

void socket_listener_stop(socket_listener_t listener)
{
    if(listener == NULL) {
        perror("Socket listener handle is NULL");
        return;
    }

    // Set the cancellation flag
    listener->cancellation = true;

    // Connect to the listener
    socket_session_t cancel_session = socket_session_connect("127.0.0.1", listener->port);
    
    // Wait for the thread to join
    macrothread_join(listener->thread);

    // Close the socket
    socket_session_close(cancel_session);

    // Destroy the thread handle
    macrothread_handle_destroy(listener->thread);

    // Destroy the listener
    free(listener);
}