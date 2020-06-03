#include "socket_session.h"
#include "socket_listener.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

#define TEST_BUFFER_LENGTH 16384

int port = 8081;
macrothread_condition_t callback_signal;
size_t bytes_received = 0;

void server_data_callback(socket_wrapper_t session)
{
    bytes_received += socket_data_length(session->data);
    socket_data_pop(session->data, socket_data_length(session->data));
    if(bytes_received == TEST_BUFFER_LENGTH) {
        macrothread_condition_signal(callback_signal);
    }
}

void server_connect_callback(socket_session_t session, void *context)
{
    TEST_NULL(context);
    session->data_callback = server_data_callback;
}

int main(void)
{
    callback_signal = macrothread_condition_init();

    // Start the listener
    socket_listener_t listener = socket_listener_start(port, 5, server_connect_callback, NULL);

    // Connect
    socket_session_t client = socket_session_create(1024);
    TEST_NOT_NULL(client);
    TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_OK);

    // Send the data
    char big_buf[TEST_BUFFER_LENGTH];
    memset(big_buf, 0, TEST_BUFFER_LENGTH * sizeof(char));
    TEST_EQUAL(socket_wrapper_write(client->socket, big_buf, TEST_BUFFER_LENGTH), SOCKET_OK);

    // Hangup
    socket_session_disconnect(client);

    // Wait for the response
    macrothread_condition_wait(callback_signal);

    // Cleanup
    socket_listener_stop(listener);
    macrothread_condition_destroy(callback_signal);
}