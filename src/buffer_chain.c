#include "buffer_chain.h"

buffer_chain_t buffer_chain_init(const size_t size)
{
    buffer_chain_t result = (buffer_chain_t)malloc(sizeof(struct buffer_chain_struct));
    result->start = buffer_link_init(size);
    result->end = result->start;
    return result;
}

void buffer_chain_write(buffer_chain_t buffer, const char* data, const size_t length)
{
    size_t bytes_written;
    const char* read_index = data;
    size_t bytes_left = length;
    while(bytes_left > 0) {
        bytes_written = buffer_link_write(buffer->end, read_index, bytes_left);
        read_index += bytes_written;
        bytes_left -= bytes_written;
        if(buffer->end->write == buffer->end->start + buffer->end->length) {
            buffer->end->next = buffer_link_init(buffer->end->length);
            buffer->end = buffer->end->next;
        }
    }
}

bool buffer_chain_contains(buffer_chain_t buffer, const char item)
{
    buffer_link_t current = buffer->start;
    while(current != buffer->end) {
        if(buffer_link_contains(current, item)) {
            return true;
        }
        current = current->next;
    }
    return buffer_link_contains(current, item);
}

const char* buffer_chain_peek(const buffer_chain_t buffer)
{
    return buffer_link_peek(buffer->start);
}

size_t buffer_chain_read(buffer_chain_t buffer, char *data, const size_t length)
{
    size_t bytes_read;
    size_t bytes_to_read = length;
    char *write_index = data;
    while(buffer->start != buffer->end && bytes_to_read > 0) {
        bytes_read = buffer_link_read(buffer->start, write_index, bytes_to_read);
        bytes_to_read -= bytes_read;
        write_index += bytes_read;
        if(buffer->start->read == buffer->start->start + buffer->start->length) {
            buffer_link_t last = buffer->start;
            buffer->start = last->next;
            buffer_link_destroy(last);
        }
    }

    if(bytes_to_read > 0) {
        bytes_read = buffer_link_read(buffer->start, write_index, bytes_to_read);
        bytes_to_read -= bytes_read;
    }
    return length - bytes_to_read;
}

void buffer_chain_destroy(buffer_chain_t buffer)
{
    buffer_link_t next;
    while(buffer->start != buffer->end) {
        next = buffer->start->next;
        buffer_link_destroy(buffer->start);
        buffer->start = next;
    }
    buffer_link_destroy(buffer->end);
    free(buffer);
}