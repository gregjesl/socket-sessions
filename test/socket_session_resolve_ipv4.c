#include "socket_session.h"
#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif
#include "test.h"

int main(void)
{
    const char *fqdn = "dns.google.com";
    socket_session_ipv4_address result;
    TEST_EQUAL(socket_session_resolve_ipv4(fqdn, result, 0), 2);
    TEST_EQUAL(strncmp(result, "8.8.", 4), 0);
    if(strncmp(&result[4], "4", 1) == 0) {
        TEST_STRING_EQUAL(result, "8.8.4.4");
        TEST_EQUAL(socket_session_resolve_ipv4(fqdn, result, 1), 2);
        TEST_STRING_EQUAL(result, "8.8.8.8");
    } else {
        TEST_STRING_EQUAL(result, "8.8.8.8");
        TEST_EQUAL(socket_session_resolve_ipv4(fqdn, result, 1), 2);
        TEST_STRING_EQUAL(result, "8.8.4.4");
    }
    TEST_EQUAL(socket_session_resolve_ipv4("bogus", result, 0), 0);
    TEST_STRING_EQUAL(result, "");
}