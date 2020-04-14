#ifndef SOCKET_SESSIONS_BUFFER_CHAIN_H
#define SOCKET_SESSIONS_BUFFER_CHAIN_H

#include "buffer_link.h"

typedef struct buffer_chain_struct
{
    buffer_link_t start;
    buffer_link_t end;
} *buffer_chain_t;

buffer_chain_t buffer_chain_init(const size_t size);
void buffer_chain_write(buffer_chain_t buffer, const char* data, const size_t length);
bool buffer_chain_contains(buffer_chain_t buffer, const char item);
const char* buffer_chain_peek(const buffer_chain_t buffer);
size_t buffer_chain_read(buffer_chain_t buffer, char *data, const size_t length);
void buffer_chain_destroy(buffer_chain_t buffer);

#endif