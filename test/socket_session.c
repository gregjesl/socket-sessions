#include "socket_session.h"
#include "socket_listener.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif

bool cancellation = false;
int port = 8081;
macrothread_condition_t callback_signal;
macrothread_condition_t server_hangup_signal, client_hangup_signal, server_finalize_signal, client_finalize_signal;
const char *test_phrase = "Hello World!\0";
socket_session_t server_session;

void client_data_callback(socket_wrapper_t session)
{
    if(socket_data_length(session->data) == strlen(test_phrase) + 1) {
        TEST_STRING_EQUAL(session->data->buffer, test_phrase);
        macrothread_condition_signal(callback_signal);
    }
}

void server_data_callback(socket_wrapper_t session)
{
    if(socket_data_length(session->data) == strlen(test_phrase) + 1) {
        TEST_STRING_EQUAL(session->data->buffer, test_phrase);

        // Send the message
        for(size_t i = 0; i < strlen(test_phrase) + 1; i++) {
            socket_wrapper_write(session, &test_phrase[i], 1);
        }
    }
}

void client_hangup_callback(socket_wrapper_t session)
{
    TEST_NOT_NULL(session);
    macrothread_condition_signal(client_hangup_signal);
}

void server_hangup_callback(socket_wrapper_t session) 
{
    TEST_NOT_NULL(session);
    macrothread_condition_signal(server_hangup_signal);
}

void client_finalize_callback(socket_wrapper_t session)
{
    TEST_NOT_NULL(session);
    macrothread_condition_signal(client_finalize_signal);
}

void server_finalize_callback(socket_wrapper_t session)
{
    TEST_NOT_NULL(session);
    macrothread_condition_signal(server_finalize_signal);
}

void server_connect_callback(socket_session_t session, void *context)
{
    server_session = session;
    TEST_STRING_EQUAL((char*)context, test_phrase);
    session->data_callback = server_data_callback;
    session->hangup_callback = server_hangup_callback;
    session->finalize_callback = server_finalize_callback;
}

int main(void)
{
    callback_signal = macrothread_condition_init();
    client_hangup_signal = macrothread_condition_init();
    client_finalize_signal = macrothread_condition_init();
    server_hangup_signal = macrothread_condition_init();
    server_finalize_signal = macrothread_condition_init();

    // Start the listener
    socket_listener_t listener = socket_listener_start(port, 5, server_connect_callback, (void*)test_phrase);

    {
        // Connect
        socket_session_t client = socket_session_create(1024);
        TEST_NOT_NULL(client);
        client->data_callback = client_data_callback;
        client->hangup_callback = client_hangup_callback;
        client->finalize_callback = client_finalize_callback;
        TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_OK);

        // Send the data
        for(size_t i = 0; i < strlen(test_phrase) + 1; i++) {
            TEST_EQUAL(socket_wrapper_write(client->socket, &test_phrase[i], 1), SOCKET_OK);
        }

        // Wait for the response
        macrothread_condition_wait(callback_signal);

        // Close the connection
        socket_wrapper_shutdown(client->socket);
        macrothread_condition_wait(client_finalize_signal);
        macrothread_condition_wait(server_hangup_signal);
        macrothread_condition_wait(server_finalize_signal);
    }

    {
        // Connect
        socket_session_t client = socket_session_create(1024);
        TEST_NOT_NULL(client);
        client->data_callback = client_data_callback;
        client->hangup_callback = client_hangup_callback;
        client->finalize_callback = client_finalize_callback;
        TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_OK);

        // Send the data
        for(size_t i = 0; i < strlen(test_phrase) + 1; i++) {
            TEST_EQUAL(socket_wrapper_write(client->socket, &test_phrase[i], 1), SOCKET_OK);
        }

        // Wait for the response
        macrothread_condition_wait(callback_signal);

        // Close the connection
        socket_wrapper_shutdown(server_session->socket);
        macrothread_condition_wait(client_hangup_signal);
        macrothread_condition_wait(client_finalize_signal);
        macrothread_condition_wait(server_finalize_signal);
    }

    // Cleanup
    socket_listener_stop(listener);
    macrothread_condition_destroy(callback_signal);
    macrothread_condition_destroy(client_hangup_signal);
    macrothread_condition_destroy(client_finalize_signal);
    macrothread_condition_destroy(server_hangup_signal);
    macrothread_condition_destroy(server_finalize_signal);
}