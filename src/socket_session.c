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

socket_session_t socket_session_init(int sock)
{
    socket_session_t result = (socket_session_t)malloc(sizeof(struct socket_session_struct));
    result->socket = sock;
    result->buffer = buffer_chain_init(1024);
    result->connected = true;
    return result;
}

int socket_session_read(socket_session_t session)
{
    struct pollfd pfd;
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

void socket_session_listen(int port, socket_session_callback callback, size_t queue, bool *cancellation)
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    socket_session_t newsession;
   
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
        
    /* Now start listening for the clients, here process will
        * go in sleep mode and will wait for the incoming connection
    */

    while(!*cancellation) {
        listen(sockfd, queue);

        if(!*cancellation) {
            clilen = sizeof(cli_addr);
   
            /* Accept actual connection from the client */
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                
            if (newsockfd < 0) {
                perror("ERROR on accept");
                exit(1);
            }

            socket_session_t newsession = socket_session_init(newsockfd);

            // Call the callback
            callback(newsession); 
        }
    }

    #ifdef WIN32
    closesocket(sockfd);
    WSACleanup();
    #else
    close(sockfd);
    #endif
}

socket_session_t socket_session_connect(const char *address, const int port)
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

    return socket_session_init(sock);
}