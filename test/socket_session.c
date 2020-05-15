#include "socket_session.h"
#include "socket_listener.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

#pragma comment(lib, "Ws2_32.lib")

bool cancellation = false;
int port = 8081;
macrothread_condition_t callback_signal;
macrothread_condition_t server_close_signal, client_close_signal;
const char *test_phrase = "Hello World!\0";
socket_session_t server_session;

void client_data_callback(socket_wrapper_t session)
{
    socket_wrapper_buffer(session);
    if(session->buffer->length == strlen(test_phrase)) {
        TEST_STRING_EQUAL(session->buffer->data, test_phrase);
        macrothread_condition_signal(callback_signal);
    }
}

void server_data_callback(socket_wrapper_t session)
{
    socket_wrapper_buffer(session);
    if(session->buffer->length == strlen(test_phrase)) {
        TEST_STRING_EQUAL(session->buffer->data, test_phrase);

        // Send the message
        for(size_t i = 0; i < strlen(test_phrase); i++) {
            socket_wrapper_write(session, &test_phrase[i], 1);
        }
    }
}

void client_close_callback(socket_wrapper_t session)
{
    macrothread_condition_signal(client_close_signal);
}

void server_close_callback(socket_wrapper_t session) 
{
    macrothread_condition_signal(server_close_signal);
}

void server_connect_callback(socket_session_t session)
{
    server_session = session;
    socket_session_start(session, server_data_callback, server_close_callback);
}

int main(void)
{
    callback_signal = macrothread_condition_init();
    server_close_signal = macrothread_condition_init();
    client_close_signal = macrothread_condition_init();

    // Start the listener
    socket_listener_t listener = socket_listener_start(port, 5, server_connect_callback);

    {
        // Connect
        socket_session_t client = socket_session_connect("127.0.0.1", port);
        TEST_NOT_NULL(client);

        // Start monitoring
        socket_session_start(client, client_data_callback, client_close_callback);

        // Send the data
        for(size_t i = 0; i < strlen(test_phrase); i++) {
            socket_wrapper_write(client->socket, &test_phrase[i], 1);
        }

        // Wait for the response
        macrothread_condition_wait(callback_signal);

        // Close the connection
        socket_session_stop(client);
        socket_session_close(client);
        macrothread_condition_wait(server_close_signal);
    }

    {
        // Connect
        socket_session_t client = socket_session_connect("127.0.0.1", port);
        TEST_NOT_NULL(client);

        // Start monitoring
        socket_session_start(client, client_data_callback, client_close_callback);

        // Send the data
        for(size_t i = 0; i < strlen(test_phrase); i++) {
            socket_wrapper_write(client->socket, &test_phrase[i], 1);
        }

        // Wait for the response
        macrothread_condition_wait(callback_signal);

        // Close the connection
        socket_session_stop(server_session);
        socket_session_close(server_session);
        macrothread_condition_wait(client_close_signal);
    }

    // Cleanup
    socket_listener_stop(listener);
    macrothread_condition_destroy(callback_signal);
    macrothread_condition_destroy(server_close_signal);
    macrothread_condition_destroy(client_close_signal);
}