#include "socket_buffer.h"
#include <string.h>

const char* error = "Error writing to buffer";

socket_buffer_t socket_buffer_init()
{
    socket_buffer_t result = (socket_buffer_t)malloc(sizeof(struct socket_buffer_struct));
    #ifdef WIN32
    result->data = (char*)malloc(1);
    *result->data = '\0';
    result->length = 0;
    #else
    result->stream = open_memstream(&result->data, &result->length);
    #endif // WIN32
    return result;
}

void socket_buffer_write(socket_buffer_t buffer, const char* data, size_t length)
{
#ifdef WIN32
    char* newbuf = (char*)malloc(buffer->length + length + 1);
    if (buffer->length > 0) {
        memcpy(newbuf, buffer->data, buffer->length);
    }
    free(buffer->data);
    memcpy(newbuf + buffer->length, data, length);
    newbuf[buffer->length + length] = '\0';
    buffer->data = newbuf;
    buffer->length += length;
#else
    if (fwrite(data, sizeof(char), length, buffer->stream) < length) {
        perror(error);
}
    fflush(buffer->stream);
#endif // !
}

void socket_buffer_splice(socket_buffer_t buffer, size_t index)
{
#ifndef WIN32
    fclose(buffer->stream);
#endif // !WIN32
    char* newbuf = NULL;
    char *oldbuf = buffer->data;
    const size_t oldlength = buffer->length;
#ifdef WIN32
    newbuf = (char*)malloc(oldlength - index + 1);
    memcpy(newbuf, oldbuf + index, oldlength - index + 1);
    buffer->length = oldlength - index;
#else
    buffer->stream = open_memstream(&newbuf, &buffer->length);
    if (index < oldlength) {
        if (fwrite(oldbuf + index, sizeof(char), oldlength - index, buffer->stream) < oldlength - index) {
            perror(error);
        }
        fflush(buffer->stream);
    }
#endif // !WIN32
    free(oldbuf);
    buffer->data = newbuf;
}

void socket_buffer_destroy(socket_buffer_t buffer)
{
#ifndef WIN32
    fclose(buffer->stream);
#endif // !WIN32
    free(buffer->data);
}