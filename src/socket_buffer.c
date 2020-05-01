#include "socket_buffer.h"
#include <string.h>

const char* error = "Error writing to buffer";

socket_buffer_t socket_buffer_init()
{
    socket_buffer_t result = (socket_buffer_t)malloc(sizeof(struct socket_buffer_struct));
    result->stream = open_memstream(&result->data, &result->length);
    return result;
}

void socket_buffer_write(socket_buffer_t buffer, const char* data, size_t length)
{
    if(fwrite(data, sizeof(char), length, buffer->stream) < length) {
        perror(error);
    }
    fflush(buffer->stream);
}

void socket_buffer_splice(socket_buffer_t buffer, size_t index)
{
    char *newbuf;
    fclose(buffer->stream);
    char *oldbuf = buffer->data;
    const size_t oldlength = buffer->length;
    buffer->stream = open_memstream(&newbuf, &buffer->length);
    if(index < oldlength) {
        if(fwrite(oldbuf + index, sizeof(char), oldlength - index, buffer->stream) < oldlength - index) {
            perror(error);
        }
        fflush(buffer->stream);
    }
    free(oldbuf);
    buffer->data = newbuf;
}

void socket_buffer_destroy(socket_buffer_t buffer)
{
    fclose(buffer->stream);
    free(buffer->data);
}