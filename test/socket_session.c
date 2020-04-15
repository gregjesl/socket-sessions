#include "socket_session.h"
#include "macrothreading_thread.h"
#include "macrothreading_condition.h"
#include "test.h"

bool cancellation = false;
int port = 8081;
macrothread_condition_t callback_signal;
macrothread_condition_t echo_signal;
socket_session_t server_session = NULL;
const char *test_phrase = "Hello World!\0";

void callback(socket_session_t session)
{
    server_session = session;
    socket_session_write(session, test_phrase, strlen(test_phrase));
    macrothread_condition_signal(callback_signal);
    macrothread_condition_wait(echo_signal);
    char echo[13];
    TEST_EQUAL(socket_session_read(session), SOCKET_SESSION_DATA_READ);
    TEST_EQUAL(buffer_chain_read(session->buffer, echo, strlen(test_phrase)), strlen(test_phrase));
    TEST_STRING_EQUAL(echo, test_phrase);
}

void listener(void* args)
{
    socket_session_listen(port, callback, 5, &cancellation);
}

int main(void)
{
    callback_signal = macrothread_condition_init();
    echo_signal = macrothread_condition_init();

    // Start the listener
    macrothread_handle_t listen_handle = macrothread_handle_init();
    macrothread_start_thread(listen_handle, listener, NULL);

    // Connect
    socket_session_t client = socket_session_connect("127.0.0.1", port);
    macrothread_condition_wait(callback_signal);
    TEST_NOT_NULL(server_session);

    // Read
    TEST_EQUAL(socket_session_read(client), SOCKET_SESSION_DATA_READ);

    // Signal shutdown
    cancellation = true;

    // Get the string
    char echo[13];
    TEST_EQUAL(buffer_chain_read(client->buffer, echo, strlen(test_phrase)), strlen(test_phrase));
    TEST_STRING_EQUAL(echo, test_phrase);

    // Write the string
    socket_session_write(client, test_phrase, strlen(test_phrase));
    macrothread_condition_signal(echo_signal);

    // Join the thread
    macrothread_join(listen_handle);

    // Cleanup
    macrothread_condition_destroy(callback_signal);
    macrothread_condition_destroy(echo_signal);
    macrothread_handle_destroy(listen_handle);
    socket_session_destroy(server_session);
}