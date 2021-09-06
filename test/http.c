#include "socket_session.h"
#include <netdb.h>
#include <arpa/inet.h>
#include "test.h"

#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

bool condition(const char *buffer, size_t len)
{
    const char *needle = strstr(buffer, "\r\n\r\n");
    return needle != NULL && needle >= buffer && needle < buffer + len;
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

    // Create the client
    socket_session_t client = socket_session_create();
    TEST_NOT_NULL(client);

    // Connect to the server
    TEST_EQUAL(socket_session_connect(client, ip, 80), SOCKET_STATE_CONNECTED);

    // Send the request
    TEST_EQUAL(socket_session_write(client, GET_REQUEST, strlen(GET_REQUEST)), strlen(GET_REQUEST));

    // Get the response
    TEST_TRUE(socket_session_buffer_until(client, condition, 4, 4096));

    // Shut down the socket
    socket_session_disconnect(client);

    // Destroy the signal
    socket_session_destroy(client);
}
