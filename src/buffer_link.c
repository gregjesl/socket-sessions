#include "buffer_link.h"
#include <string.h>

buffer_link_t buffer_link_init(const size_t length)
{
    buffer_link_t result = (buffer_link_t)malloc(sizeof(struct buffer_link_struct));
    result->start = (char*)malloc(sizeof(char) * length);
    result->read = result->start;
    result->write = result->start;
    result->length = length;
    result->next = NULL;
    return result;
}

size_t buffer_link_write(buffer_link_t buffer, const char *data, const size_t length)
{
    const size_t max_bytes_to_copy = (buffer->start + buffer->length) - buffer->write;
    const size_t bytes_to_copy = max_bytes_to_copy < length ? max_bytes_to_copy : length;
    
    if(bytes_to_copy > 0) {
        memcpy(buffer->write, data, bytes_to_copy * sizeof(char));
        buffer->write += bytes_to_copy;
    }
    return bytes_to_copy;
}

bool buffer_link_contains(const buffer_link_t buffer, const char item)
{
    const char *index = buffer->read;
    while(index < buffer->write) {
        if(*index == item)
            return true;
        index++;
    }
    return false;
}

const char* buffer_link_peek(const buffer_link_t buffer)
{
    return buffer->read < buffer->write ? buffer->read : NULL;
}

size_t buffer_link_read(buffer_link_t buffer, char *data, const size_t length)
{
    const size_t max_bytes_to_read = buffer->write - buffer->read;
    const size_t bytes_to_read = max_bytes_to_read < length ? max_bytes_to_read : length;
    memcpy(data, buffer->read, bytes_to_read * sizeof(char));
    buffer->read += bytes_to_read;
    return bytes_to_read;
}

void buffer_link_destroy(buffer_link_t buffer)
{
    free(buffer->start);
    free(buffer);
}