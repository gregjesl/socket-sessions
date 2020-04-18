#include "socket_listener.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

void socket_listener_thread(void *arg)
{
    SOCKET newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    socket_session_t newsession;

    socket_listener_t handle = (socket_listener_t)arg;

    while(!handle->cancellation) {
        listen(handle->sockfd, handle->queue);

        if(!handle->cancellation) {
            clilen = sizeof(cli_addr);
   
            /* Accept actual connection from the client */
            newsockfd = accept(handle->sockfd, (struct sockaddr *)&cli_addr, &clilen);
                
            if (newsockfd < 0) {
                perror("ERROR on accept");
                exit(1);
            }

            socket_session_t newsession = socket_session_init();
            newsession->connected = true;
            newsession->socket = newsockfd;

            // Call the callback
            handle->connection_callback(newsession); 
        }
    }
}

socket_listener_t socket_listener_start(int port, int queue, socket_session_callback callback)
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;

    // Create the handle
    socket_listener_t handle = (socket_listener_t)malloc(sizeof(struct socket_listener_struct));
    handle->port = port;
    handle->queue = queue;
    handle->connection_callback = callback;
    handle->cancellation = false;
   
    /* First call to socket() function */
    handle->sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (handle->sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

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
        exit(1);
    }

    // Set the cancellation flag
    listener->cancellation = true;

    // Connect to the listener
    socket_session_t cancel_session = socket_session_init();
    socket_session_connect(cancel_session, "127.0.0.1", listener->port);
    
    // Wait for the thread to join
    macrothread_join(listener->thread);

    // Close the socket
    socket_session_close(cancel_session);

    // Destroy the socket
    socket_session_destroy(cancel_session);
    #ifdef WIN32
    closesocket(listener->sockfd);
    WSACleanup();
    #else
    close(listener->sockfd);
    #endif

    // Destroy the thread handle
    macrothread_handle_destroy(listener->thread);

    // Destroy the listener
    free(listener);
}