#include "socket_session.h"
#include "socket_listener.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

int port = 8081;
const char *test_phrase = "Hello World!\0";
socket_session_t server_session = NULL;

macrothread_condition_t server_connect_signal;
void server_connect_callback(socket_session_t session, void *context)
{
    server_session = session;
    TEST_STRING_EQUAL(test_phrase, (char*)context);
    macrothread_condition_signal(server_connect_signal);
}

int main(void)
{
    // Initialize the signals
    server_connect_signal = macrothread_condition_init();

    // Start the listener
    socket_listener_t listener = socket_listener_start(port, 5, server_connect_callback, (void*)test_phrase);
    TEST_NOT_NULL(listener);

    {
        // Create the client
        socket_session_t client = socket_session_create();
        TEST_NOT_NULL(client);

        // Connect to the server
        TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_STATE_CONNECTED);
        macrothread_condition_wait(server_connect_signal);

        // Send the data
        for(size_t i = 0; i < strlen(test_phrase) + 1; i++) {
            TEST_EQUAL(socket_session_write(client, &test_phrase[i], 1), 1);
        }

        // Buffer the data
        TEST_EQUAL(socket_session_buffer(server_session, strlen(test_phrase) + 1, strlen(test_phrase) + 1), strlen(test_phrase) + 1);

        // Compare the results
        TEST_STRING_EQUAL(test_phrase, server_session->buffer);

        // Shut down the connection
        TEST_EQUAL(socket_session_write(client, test_phrase, strlen(test_phrase) + 1), strlen(test_phrase) + 1);
        socket_session_shutdown(client);
        TEST_EQUAL(client->state, SOCKET_STATE_SHUTDOWN);

        // Verify shutdown
        #ifndef unix
        do
        {
            socket_session_wait(server_session);
        } while (server_session->state != SOCKET_STATE_PEER_CLOSED);
        #endif

        // Read the rest of the buffer
        socket_session_flush(server_session, server_session->buffer_len);
        TEST_EQUAL(server_session->buffer_len, 0);
        socket_session_buffer(server_session, strlen(test_phrase) + 1, strlen(test_phrase) + 1);
        TEST_STRING_EQUAL(test_phrase, server_session->buffer);

        // Verify closure
        socket_session_disconnect(client);
        TEST_EQUAL(client->state, SOCKET_STATE_CLOSED);
        socket_session_wait(server_session);
        char buf;
        TEST_EQUAL(socket_session_read(server_session, &buf, 1), 0);
        TEST_EQUAL(server_session->state, SOCKET_STATE_CLOSED);

        // Cleanup
        socket_session_destroy(client);
        socket_session_destroy(server_session);
    }
    
    {
        // Create the client
        socket_session_t client = socket_session_create();
        TEST_NOT_NULL(client);

        // Connect to the server
        TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_STATE_CONNECTED);
        macrothread_condition_wait(server_connect_signal);

        // Send the data
        for(size_t i = 0; i < strlen(test_phrase) + 1; i++) {
            TEST_EQUAL(socket_session_write(server_session, &test_phrase[i], 1), 1);
        }

        // Buffer the data
        TEST_EQUAL(socket_session_buffer(client, strlen(test_phrase) + 1, strlen(test_phrase) + 1), strlen(test_phrase) + 1);

        // Compare the results
        TEST_STRING_EQUAL(test_phrase, client->buffer);

        // Shut down the connection
        TEST_EQUAL(socket_session_write(server_session, test_phrase, strlen(test_phrase) + 1), strlen(test_phrase) + 1);
        socket_session_shutdown(server_session);
        TEST_EQUAL(server_session->state, SOCKET_STATE_SHUTDOWN);

        // Verify shutdown
        #ifndef unix
        do
        {
            socket_session_wait(client);
        } while (client->state != SOCKET_STATE_PEER_CLOSED);
        #endif

        // Read the rest of the buffer
        socket_session_flush(client, client->buffer_len);
        TEST_EQUAL(client->buffer_len, 0);
        socket_session_buffer(client, strlen(test_phrase) + 1, strlen(test_phrase) + 1);
        TEST_STRING_EQUAL(test_phrase, client->buffer);

        // Verify closure
        socket_session_disconnect(server_session);
        TEST_EQUAL(server_session->state, SOCKET_STATE_CLOSED);
        socket_session_wait(client);
        char buf;
        TEST_EQUAL(socket_session_read(client, &buf, 1), 0);
        TEST_EQUAL(client->state, SOCKET_STATE_CLOSED);

        // Cleanup
        socket_session_destroy(client);
        socket_session_destroy(server_session);
    }

    // Cleanup
    socket_listener_stop(listener);
    macrothread_condition_destroy(server_connect_signal);
}