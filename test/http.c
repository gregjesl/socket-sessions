#include "socket_session.h"
#include "macrothreading_condition.h"
#include <netdb.h>
#include<arpa/inet.h>
#include "test.h"

#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

macrothread_condition_t data_signal;

void client_data_callback(socket_wrapper_t session)
{
    char *ret = strstr(session->data->buffer, "\r\n\r\n");
    if(ret != NULL && ret < session->data->write_buffer) {
        macrothread_condition_signal(data_signal);
    }
}

int main(void)
{
    struct hostent *server_host = NULL;
    struct in_addr **addr_list = NULL;
    char ip[100];
    
    // Resolve the IP address
    server_host = gethostbyname("example.com");
    TEST_NOT_NULL(server_host);
    addr_list = (struct in_addr **) server_host->h_addr_list;
    TEST_NOT_NULL(addr_list[0]);
    strcpy(ip , inet_ntoa(*addr_list[0]));

    // Create the signal
    data_signal = macrothread_condition_init();

    // Create the client
    socket_session_t client = socket_session_create(1024 * 1024);
    TEST_NOT_NULL(client);
    client->data_callback = client_data_callback;

    // Connect to the server
    TEST_EQUAL(socket_session_connect(client, ip, 80), SOCKET_OK);

    // Send the request
    TEST_EQUAL(socket_wrapper_write(client->socket, GET_REQUEST, strlen(GET_REQUEST)), SOCKET_OK);

    // Wait for the response
    macrothread_condition_wait(data_signal);

    // Shut down the socket
    socket_session_disconnect(client);

    // Destroy the signal
    macrothread_condition_destroy(data_signal);
}
