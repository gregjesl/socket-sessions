#include "socket_data.h"
#include "test.h"

int main(void)
{
    const size_t max_length = 12;

    // Create a new data structure
    socket_data_t data = socket_data_init(max_length);
    TEST_NOT_NULL(data);
    TEST_EQUAL(data->buffer_length, 0);
    TEST_EQUAL(data->max_buffer_length, max_length);

    // Test for a null arguement
    const char one = 1;
    TEST_EQUAL(socket_data_push(NULL, &one, 1), SOCKET_ERROR_NULL_ARGUEMENT);
    TEST_EQUAL(socket_data_push(data, NULL, 1), SOCKET_ERROR_NULL_ARGUEMENT);

    // Populate the data structure
    for(char i = 0; i < max_length; i++) {
        TEST_EQUAL(socket_data_push(data, &i, 1), SOCKET_OK);
    }
    
    TEST_EQUAL(socket_data_push(data, NULL, 0), SOCKET_OK);
    TEST_EQUAL(socket_data_push(data, &one, 1), SOCKET_ERROR_BUFFER_OVERFLOW);

    // Verify the data structure
    for(char i = 0; i < max_length; i++) {
        TEST_EQUAL(*data->buffer, i);
        TEST_EQUAL(data->buffer_length, max_length - i);
        TEST_EQUAL(socket_data_pop(data, 1), SOCKET_OK);
    }

    TEST_NULL(data->buffer);
    TEST_EQUAL(data->buffer_length, 0);

    socket_data_destroy(data);
}