#include "socket_session.h"
#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include "test.h"

#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

bool condition(const char *buffer, size_t len)
{
    const char *needle = strstr(buffer, "\r\n\r\n");
    return needle != NULL && needle >= buffer && needle < buffer + len;
}

int main(void)
{
    socket_session_ipv4_address ip;
    
    // Resolve the IP address
    TEST_TRUE(socket_session_resolve_ipv4("example.com", ip, 0) > 0);

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

    // Cleanup
    socket_session_cleanup();
}
