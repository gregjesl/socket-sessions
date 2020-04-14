#include "buffer_chain.h"
#include "test.h"

const char* test_string = "Hello World\0";

int main(void)
{
    // Initalize the buffer
    buffer_chain_t buf = buffer_chain_init(2);
    TEST_NOT_NULL(buf);
    TEST_NOT_NULL(buf->start);
    TEST_NOT_NULL(buf->end);
    TEST_EQUAL(buf->start, buf->end);

    // Write data to the buffer
    buffer_chain_write(buf, test_string, strlen(test_string));

    // Contains
    TEST_TRUE(buffer_chain_contains(buf, 'd'));
    TEST_FALSE(buffer_chain_contains(buf, 'z'));

    for(size_t i = 0; i < 11; i++) {

        // Peek
        TEST_EQUAL(*buffer_chain_peek(buf), test_string[i]);

        // Read
        char echo;
        TEST_EQUAL(buffer_chain_read(buf, &echo, 1), 1);
        TEST_EQUAL(echo, test_string[i]);
    }

    TEST_EQUAL(buf->start, buf->end);

    // Destroy
    buffer_chain_destroy(buf);
}