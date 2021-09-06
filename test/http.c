#include "socket_session.h"
#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
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
    char ip[100];
    
    // Resolve the IP address
    #ifdef WIN32
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct sockaddr_in  *sockaddr_ipv4;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    TEST_EQUAL(getaddrinfo("example.com", "80", &hints, &result), 0);
    sockaddr_ipv4 = (struct sockaddr_in *) result->ai_addr;
    strcpy(ip, inet_ntoa(sockaddr_ipv4->sin_addr));
    #else
    struct hostent *server_host = NULL;
    struct in_addr **addr_list = NULL;
    server_host = gethostbyname("example.com");
    TEST_NOT_NULL(server_host);
    addr_list = (struct in_addr **) server_host->h_addr_list;
    TEST_NOT_NULL(addr_list[0]);
    strcpy(ip , inet_ntoa(*addr_list[0]));
    #endif

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
