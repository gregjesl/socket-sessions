#include "socket_session.h"
#include "socket_listener.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

bool cancellation = false;
int port = 8081;
macrothread_condition_t callback_signal;
macrothread_condition_t close_signal;
const char *test_phrase = "Hello World!\0";
socket_monitor_t server_monitor;

void client_data_callback(socket_session_t session)
{
    char echo[13];
    TEST_EQUAL(buffer_chain_read(session->buffer, echo, strlen(test_phrase)), strlen(test_phrase));
    TEST_STRING_EQUAL(echo, test_phrase);
    macrothread_condition_signal(callback_signal);
}

void server_data_callback(socket_session_t session)
{
    // Verify the message
    char echo[13];
    TEST_EQUAL(buffer_chain_read(session->buffer, echo, strlen(test_phrase)), strlen(test_phrase));
    TEST_STRING_EQUAL(echo, test_phrase);

    // Send the message
    socket_session_write(session, test_phrase, strlen(test_phrase));
}

void server_close_callback(socket_session_t session) 
{
    macrothread_condition_signal(close_signal);
}

void server_connect_callback(socket_session_t session)
{
    server_monitor = socket_session_monitor(session, server_data_callback, server_close_callback);
}

int main(void)
{
    callback_signal = macrothread_condition_init();
    close_signal = macrothread_condition_init();

    // Start the listener
    socket_listener_t listener = socket_listener_start(port, 5, server_connect_callback); 

    // Connect
    socket_session_t client = socket_session_init();
    socket_session_connect(client, "127.0.0.1", port);
    socket_monitor_t client_monitor = socket_session_monitor(client, client_data_callback, NULL);

    // Send the data
    socket_session_write(client, test_phrase, strlen(test_phrase));

    // Wait for the response
    macrothread_condition_wait(callback_signal);

    // Close the connection
    socket_session_stop_monitor(client_monitor);
    socket_session_close(client);
    macrothread_condition_wait(close_signal);


    // Cleanup
    socket_session_stop_monitor(server_monitor);
    socket_listener_stop(listener);
    macrothread_condition_destroy(callback_signal);
    macrothread_condition_destroy(close_signal);
    socket_session_destroy(client);
}