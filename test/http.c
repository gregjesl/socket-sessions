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
    char ip[NI_MAXHOST];
    
    // Resolve the IP address
    #ifdef WIN32
    winsock_init();
    int                  errno;
    struct addrinfo      hints;  //prefered addr type(connection)
    struct addrinfo  *   list = NULL;   //list of addr structs
    struct addrinfo  *   addrptr = NULL;//the one i am gonna use

    char *servname = "example.com";

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags                = 0;
    hints.ai_family               = AF_INET;
    hints.ai_socktype             = SOCK_RAW;
    hints.ai_protocol             = IPPROTO_ICMP;

    if ((errno = getaddrinfo(servname, 0, &hints, &list))<0){
        fprintf(stderr, "addrinfo error, lookup fail:  %s", gai_strerror(errno));
        exit(1);
    }

    addrptr = list;

    if (addrptr != NULL) {
        getnameinfo(addrptr->ai_addr, addrptr->ai_addrlen, ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
        printf("%s can be reached at %s\n", servname, ip);
    }

    freeaddrinfo(list);
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
