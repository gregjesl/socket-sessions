#include "socket_wrapper.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#define poll(a,b,c) WSAPoll(a, b, c)
#else
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#endif // WIN32


socket_wrapper_t socket_wrapper_init(SOCKET id)
{
    socket_wrapper_t result = (socket_wrapper_t)malloc(sizeof(struct socket_wrapper_struct));
    result->id = id;
    result->buffer = socket_buffer_init();
    result->connected = true;
    result->activity_flag = false;
    result->closure_requested = false;
    result->context = NULL;
    return result;
}

ssize_t socket_wrapper_read(socket_wrapper_t wrapper, char *buffer, size_t max_bytes)
{
    struct pollfd pfd;
    ssize_t bytes_read;
    size_t total_bytes_read = 0;

    if(!wrapper->connected) 
        return SOCKET_SESSION_CLOSED;

    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = wrapper->id;
    pfd.events = POLLIN;

    while(poll(&pfd, 1, 0) > 0 && total_bytes_read < max_bytes)
    {
        if(pfd.revents & POLLIN) {
            #ifdef WIN32
            bytes_read = recv(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read, 0);
            #else
            bytes_read = read(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read);
            #endif

            total_bytes_read += bytes_read;
            wrapper->activity_flag = true;
        }

        if(pfd.revents & POLLHUP) {
            wrapper->connected = false;
            return total_bytes_read > 0 ? total_bytes_read : SOCKET_SESSION_CLOSED;
            break;
        }

        if(pfd.revents & POLLERR) {
            perror("Poll error");
            return SOCKET_SESSION_ERROR;
        }
    }
    
    return total_bytes_read;
}

ssize_t socket_wrapper_buffer(socket_wrapper_t wrapper)
{
    const size_t shovel_size = 128;
    ssize_t result = 0;
    ssize_t shovel_result;
    char *shovel = (char*)malloc(shovel_size * sizeof(char));
    do
    {
        shovel_result = socket_wrapper_read(wrapper, shovel, shovel_size);
        if(shovel_result > 0) {
            socket_buffer_write(wrapper->buffer, shovel, shovel_result);
        }
    } while (shovel_result > 0);

    free(shovel);
    return shovel_result < 0 ? shovel_result : result;
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
                bytes_written == WSAECONNRESET
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
        session->activity_flag = true;
    }

    return SOCKET_ACTION_COMPLETE;
}

void socket_wrapper_destroy(socket_wrapper_t wrapper)
{
    socket_buffer_destroy(wrapper->buffer);
    free(wrapper);
}