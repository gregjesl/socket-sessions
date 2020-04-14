#include "buffer_link.h"
#include "test.h"

const char* test_string = "Hello World\0";
const size_t length = 16;

int main(void)
{
    // Initalize the buffer
    buffer_link_t buf = buffer_link_init(length);
    TEST_NOT_NULL(buf);
    TEST_NOT_NULL(buf->start);
    TEST_NOT_NULL(buf->read);
    TEST_NOT_NULL(buf->write);
    TEST_EQUAL(buf->start, buf->read);
    TEST_EQUAL(buf->read, buf->write);
    TEST_EQUAL(buf->length, length);
    TEST_NULL(buf->next);

    // Write data to the buffer
    TEST_EQUAL(buffer_link_write(buf, test_string, strlen(test_string)), strlen(test_string));

    // Contains
    TEST_TRUE(buffer_link_contains(buf, 'd'));
    TEST_FALSE(buffer_link_contains(buf, 'z'));

    // Peek
    TEST_EQUAL(*buffer_link_peek(buf), 'H');

    // Read
    char echo[12];
    TEST_EQUAL(buffer_link_read(buf, echo, strlen(test_string)), strlen(test_string));
    TEST_STRING_EQUAL(echo, test_string);

    // Write to the end
    TEST_EQUAL(buffer_link_write(buf, test_string, strlen(test_string)), 16 - strlen(test_string));

    // Destroy
    buffer_link_destroy(buf);
}