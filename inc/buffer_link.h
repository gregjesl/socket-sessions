#ifndef SOCKET_SESSIONS_BUFFER_LINK_H
#define SOCKET_SESSIONS_BUFFER_LINK_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct buffer_link_struct
{
    char *start;
    char *read;
    char *write;
    size_t length;
    struct buffer_link_struct *next;
} *buffer_link_t;

buffer_link_t buffer_link_init(const size_t length);
size_t buffer_link_write(buffer_link_t buffer, const char *data, const size_t length);
bool buffer_link_contains(buffer_link_t buffer, const char item);
const char* buffer_link_peek(const buffer_link_t buffer);
size_t buffer_link_read(buffer_link_t buffer, char *data, const size_t length);
void buffer_link_destroy(buffer_link_t buffer);

#endif