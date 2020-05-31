#include "socket_session.h"
#include "socket_listener.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

#pragma comment(lib, "Ws2_32.lib")

int port = 8081;
macrothread_condition_t server_hangup_signal, client_hangup_signal, server_timeout_signal, client_timeout_signal;

void client_hangup_callback(socket_wrapper_t session)
{
    macrothread_condition_signal(client_hangup_signal);
}

void server_hangup_callback(socket_wrapper_t session) 
{
    macrothread_condition_signal(server_hangup_signal);
}

void client_timeout_callback(socket_wrapper_t session)
{
    macrothread_condition_signal(client_timeout_signal);
}

void server_timeout_callback(socket_wrapper_t session)
{
    macrothread_condition_signal(server_timeout_signal);
}

void server_connect_callback(socket_session_t session, void *context)
{
    session->hangup_callback = server_hangup_callback;
    session->timeout_callback = server_timeout_callback;
    session->socket->timeout = 0.2f;
}

int main(void)
{
    client_hangup_signal = macrothread_condition_init();
    client_timeout_signal = macrothread_condition_init();
    server_hangup_signal = macrothread_condition_init();
    server_timeout_signal = macrothread_condition_init();

    // Start the listener
    socket_listener_t listener = socket_listener_start(port, 5, server_connect_callback, NULL);

    {
        // Connect
        socket_session_t client = socket_session_create(1024);
        TEST_NOT_NULL(client);
        client->hangup_callback = client_hangup_callback;
        client->timeout_callback = client_timeout_callback;
        client->socket->timeout = 0.1f;
        TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_OK);

        // Wait for the timeout
        macrothread_condition_wait(client_timeout_signal);
        macrothread_condition_wait(server_hangup_signal);
    }

    {
        // Connect
        socket_session_t client = socket_session_create(1024);
        TEST_NOT_NULL(client);
        client->hangup_callback = client_hangup_callback;
        client->timeout_callback = client_timeout_callback;
        TEST_EQUAL(socket_session_connect(client, "127.0.0.1", port), SOCKET_OK);

        // Wait for the timeout
        macrothread_condition_wait(server_timeout_signal);
        macrothread_condition_wait(client_hangup_signal);
    }

    // Cleanup
    socket_listener_stop(listener);
    macrothread_condition_destroy(client_hangup_signal);
    macrothread_condition_destroy(client_timeout_signal);
    macrothread_condition_destroy(server_hangup_signal);
    macrothread_condition_destroy(server_timeout_signal);
}