#include "socket_wrapper.h"
#include "socket_error.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#define poll(a,b,c) WSAPoll(a, b, c)
#define SHUT_WR SD_SEND
#else
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#endif // WIN32


socket_wrapper_t socket_wrapper_init(SOCKET id, size_t max_buffer_length)
{
    socket_wrapper_t result = (socket_wrapper_t)malloc(sizeof(struct socket_wrapper_struct));
    result->id = id;
    result->state = SOCKET_STATE_CLOSED;
    result->data = socket_data_init(max_buffer_length);
    result->last_activity = 0;
    result->timeout = 0.0f;
    result->context = NULL;
    return result;
}

ssize_t socket_wrapper_read(socket_wrapper_t wrapper, char *buffer, size_t max_bytes, int poll_period_ms)
{
    struct pollfd pfd;
    ssize_t bytes_read;
    size_t total_bytes_read = 0;

    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = wrapper->id;
    pfd.events = POLLIN;

    while(
        wrapper->state == SOCKET_STATE_CONNECTED
        && 
        total_bytes_read < max_bytes
        &&
        poll(&pfd, 1, poll_period_ms) > 0 
    )
    {
        if(pfd.revents & POLLIN) {
            #ifdef WIN32
            bytes_read = recv(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read, 0);
            #else
            bytes_read = read(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read);
            #endif

            if(bytes_read < 0) {
                wrapper->state = SOCKET_STATE_ERROR;
                return SOCKET_ERROR_READ;
            }

            total_bytes_read += bytes_read;
            wrapper->last_activity = 0;
        }

        if(pfd.revents & POLLHUP || pfd.revents & POLLNVAL || bytes_read == 0) {
            wrapper->state = (bytes_read > 0) ? SOCKET_STATE_PEER_CLOSED : SOCKET_STATE_CLOSED;
            break;
        }

        if(pfd.revents & POLLERR) {
            wrapper->state = SOCKET_STATE_ERROR;
            return SOCKET_ERROR_POLL;
        }
    }

    if(total_bytes_read == max_bytes) {
        wrapper->last_activity += poll_period_ms;
        return total_bytes_read;
    }

    if(wrapper->state == SOCKET_STATE_PEER_CLOSED) {
        do
        {
            #ifdef WIN32
            bytes_read = recv(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read, 0);
            #else
            bytes_read = read(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read);
            #endif

            if(bytes_read < 0) {
                wrapper->state = SOCKET_STATE_ERROR;
                return SOCKET_ERROR_READ;
            } else if(bytes_read == 0) {
                wrapper->state = SOCKET_STATE_CLOSED;
            }

            total_bytes_read += bytes_read;
            wrapper->last_activity = 0;
        } while (bytes_read > 0 && total_bytes_read < max_bytes);
    }

    wrapper->last_activity += poll_period_ms;
    return total_bytes_read;
}

int socket_wrapper_write(socket_wrapper_t session, const char *data, const size_t length)
{
    ssize_t bytes_written = 0;
    const char *write_index = data;
    size_t bytes_to_write = length;

    if(session->state != SOCKET_STATE_CONNECTED) {
        return SOCKET_ERROR_CLOSED;
    }

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
                session->state = SOCKET_STATE_PEER_CLOSED;
                return SOCKET_ERROR_CLOSED;
            } else {
                session->state = SOCKET_STATE_ERROR;
                return SOCKET_ERROR_WRITE;;
            }
            #else
            if(errno == EPIPE) {
                session->state = SOCKET_STATE_PEER_CLOSED;
                return SOCKET_ERROR_CLOSED;
            } else {
                session->state = SOCKET_STATE_ERROR;
                return SOCKET_ERROR_WRITE;
            }
            #endif
        }

        bytes_to_write -= bytes_written;
        write_index += bytes_written;
        session->last_activity = 0;
    }

    return SOCKET_OK;
}

int socket_wrapper_shutdown(socket_wrapper_t wrapper)
{
    if(wrapper->state == SOCKET_STATE_CLOSED) {
        return SOCKET_ERROR_CLOSED;
    }
    if(wrapper->state == SOCKET_STATE_CONNECTED) {
        wrapper->state = SOCKET_STATE_SHUTDOWN;
        shutdown(wrapper->id, SHUT_WR);
        return SOCKET_OK;
    }
    return SOCKET_ERROR_CONFLICT;
}

void socket_wrapper_destroy(socket_wrapper_t wrapper)
{
    socket_data_destroy(wrapper->data);
    free(wrapper);
}