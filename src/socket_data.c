#include "socket_data.h"
#include <stdio.h>
#include <string.h>

socket_data_t socket_data_init(size_t max_buffer_length)
{
    socket_data_t result = (socket_data_t)malloc(sizeof(struct socket_data_struct));
    result->buffer = NULL;
    result->buffer_length = 0;
    result->max_buffer_length = max_buffer_length;
    return result;
}

int socket_data_push(socket_data_t data, const char *buffer, size_t length)
{
    if(data == NULL) return SOCKET_ERROR_NULL_ARGUEMENT;
    if(buffer == NULL && length > 0) return SOCKET_ERROR_NULL_ARGUEMENT;
    if(length == 0) return SOCKET_OK;

    const size_t newlen = data->buffer_length + length;
    if(newlen > data->max_buffer_length) return SOCKET_ERROR_BUFFER_OVERFLOW;

    // Create the new buffer
    char *newbuf = (char*)malloc((data->buffer_length + length) * sizeof(char));
    if(newbuf == NULL) return SOCKET_ERROR_MALLOC_FAIL;

    // Copy the data
    memcpy(newbuf, data->buffer, data->buffer_length * sizeof(char));
    memcpy(newbuf + data->buffer_length, buffer, length * sizeof(char));

    // Swap the buffers
    if(data->buffer != NULL) {
        free(data->buffer);
    }
    data->buffer = newbuf;
    data->buffer_length += length;
    return SOCKET_OK;
}

int socket_data_pop(socket_data_t data, size_t length)
{
    if(data == NULL) return SOCKET_ERROR_NULL_ARGUEMENT;

    if(length == 0) return SOCKET_OK;
    if(length > data->buffer_length) return SOCKET_ERROR_BUFFER_UNDERFLOW;

    const size_t newlength = data->buffer_length - length;
    char *newbuf = NULL;
    if(newlength > 0) {
        newbuf = newlength > 0 ? (char*)malloc(newlength * sizeof(char)) : NULL;
        if(newbuf == NULL) return SOCKET_ERROR_MALLOC_FAIL;
        const char *read_index = data->buffer + length;
        memcpy(newbuf, read_index, (data->buffer_length - length) * sizeof(char));
    }

    if(data->buffer != NULL) {
        free(data->buffer);
    }
    data->buffer = newbuf;
    data->buffer_length = newlength;
    return SOCKET_OK;
}

void socket_data_destroy(socket_data_t data)
{
    if(data == NULL) return;
    if(data->buffer != NULL) {
        free(data->buffer);
    }
    free(data);
}