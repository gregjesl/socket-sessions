#include "socket_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

socket_data_t socket_data_init(size_t max_buffer_length)
{
    if(max_buffer_length == 0) return NULL;
    socket_data_t result = (socket_data_t)malloc(sizeof(struct socket_data_struct));
    result->buffer = (char*)malloc(max_buffer_length * sizeof(char));
    result->write_buffer = result->buffer;
    result->buffer_length = max_buffer_length;
    return result;
}

size_t socket_data_length(socket_data_t data)
{
    assert(data != NULL);
    assert(data->write_buffer >= data->buffer);
    assert(data->write_buffer - data->buffer <= data->buffer_length);
    return data->write_buffer - data->buffer;
}

int socket_data_push(socket_data_t data, const char *buffer, size_t length)
{
    if(data == NULL) return SOCKET_ERROR_NULL_ARGUEMENT;
    if(buffer == NULL && length > 0) return SOCKET_ERROR_NULL_ARGUEMENT;
    if(length == 0) return SOCKET_OK;
    if(socket_data_length(data) + length > data->buffer_length) return SOCKET_ERROR_BUFFER_OVERFLOW;
    
    memcpy(data->write_buffer, buffer, length * sizeof(char));
    data->write_buffer += length;
    return SOCKET_OK;
}

int socket_data_pop(socket_data_t data, size_t length)
{
    if(data == NULL) return SOCKET_ERROR_NULL_ARGUEMENT;
    if(length == 0) return SOCKET_OK;
    if(socket_data_length(data) < length) return SOCKET_ERROR_BUFFER_UNDERFLOW;

    const size_t bytes_to_move = socket_data_length(data) - length;
    memmove(data->buffer, data->buffer + length, bytes_to_move);
    data->write_buffer -= length;
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