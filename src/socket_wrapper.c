#include "socket_wrapper.h"
#include "socket_error.h"
#include <assert.h>
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


socket_wrapper_t socket_wrapper_init(SOCKET id, size_t max_buffer_length)
{
    socket_wrapper_t result = (socket_wrapper_t)malloc(sizeof(struct socket_wrapper_struct));
    result->id = id;
    result->state = 0;
    result->data = socket_data_init(max_buffer_length);
    result->connected = true;
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

    if(!wrapper->connected) 
        return SOCKET_ERROR_CLOSED;

    memset(&pfd, 0, sizeof(struct pollfd));
    pfd.fd = wrapper->id;
    pfd.events = POLLIN;

    while(poll(&pfd, 1, poll_period_ms) > 0 && total_bytes_read < max_bytes)
    {
        wrapper->state = pfd.revents;
        if(pfd.revents & POLLIN) {
            #ifdef WIN32
            bytes_read = recv(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read, 0);
            #else
            bytes_read = read(wrapper->id, buffer + total_bytes_read, max_bytes - total_bytes_read);
            #endif

            total_bytes_read += bytes_read;
            wrapper->last_activity = 0;
        }

        if(pfd.revents & POLLHUP || pfd.revents & POLLNVAL) {
            wrapper->connected = false;
            return total_bytes_read > 0;
        }

        if(pfd.revents & POLLERR) {
            return SOCKET_ERROR_POLL;
        }
    }

    wrapper->last_activity += poll_period_ms;
    return total_bytes_read;
}

ssize_t socket_wrapper_buffer(socket_wrapper_t wrapper, int poll_period_ms)
{
    if(wrapper->data->buffer_length >= wrapper->data->max_buffer_length) return 0;
    const size_t buffer_length = wrapper->data->max_buffer_length - wrapper->data->buffer_length;
    char *buffer = (char*)malloc(buffer_length * sizeof(char));
    if(buffer == NULL) return SOCKET_ERROR_MALLOC_FAIL;
    char *write_index = buffer;
    size_t bytes_written = 0;
    ssize_t read_result = 0;

    while(bytes_written < buffer_length) {
        assert(buffer_length > bytes_written);
        read_result = socket_wrapper_read(wrapper, write_index, buffer_length - bytes_written, poll_period_ms);
        if(read_result <= 0) {
            break;
        }
        bytes_written += read_result;
        write_index += read_result;
    }
    
    if(bytes_written > 0) {
        socket_data_push(wrapper->data, buffer, bytes_written);
    }
    free(buffer);
    return read_result < 0 ? read_result : bytes_written;
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
            return bytes_written == EPIPE ? SOCKET_ERROR_CLOSED : SOCKET_ERROR_WRITE;
            #endif
        }

        bytes_to_write -= bytes_written;
        write_index += bytes_written;
        session->last_activity = 0;
    }

    return SOCKET_OK;
}

int socket_wrapper_close(socket_wrapper_t wrapper)
{
    if(wrapper->connected) {
        shutdown(wrapper->id, SHUT_WR);
        ssize_t recv_result = 0;
        do
        {
            recv_result = read(wrapper->id, NULL, 1);
            if(recv_result < 0) {
                break;
            }
        } while (recv_result > 0);
    }
    #ifdef WIN32
    closesocket(wrapper->id);
    #else
    close(wrapper->id);
    #endif
    wrapper->connected = false;
    return SOCKET_OK;
}

void socket_wrapper_destroy(socket_wrapper_t wrapper)
{
    socket_data_destroy(wrapper->data);
    free(wrapper);
}