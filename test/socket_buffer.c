#include "socket_buffer.h"
#include "test.h"

const char *test_phrase = "Hello World!";

int main(void)
{
    // Create the buffer
    socket_buffer_t buffer = socket_buffer_init();
    
    // Write to the buffer
    socket_buffer_write(buffer, test_phrase, strlen(test_phrase));

    // Verify the length
    TEST_EQUAL(buffer->length, strlen(test_phrase));

    // Verify the value
    TEST_STRING_EQUAL(buffer->data, test_phrase);

    // Split the buffer
    socket_buffer_splice(buffer, 6);
    
    // Verify the new length
    TEST_EQUAL(buffer->length, strlen("World!"));

    // Verify the new value
    TEST_STRING_EQUAL(buffer->data, "World!");

    // Destroy the buffer
    socket_buffer_destroy(buffer);
}