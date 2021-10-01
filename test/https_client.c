/* client-tls.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* wolfSSL */
#include "wolfssl/options.h"
#include "wolfssl/ssl.h"

/* Socket session */
#include "socket_session.h"

#define CERT_FILE "./cert.pem"
#include "test.h"

bool condition(const char *buffer, size_t len)
{
    const char *needle = strstr(buffer, "\r\n\r\n");
    return needle != NULL && needle >= buffer && needle < buffer + len;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    socket_session_ipv4_address ip;

    /* Get the IP address */
    TEST_TRUE(socket_session_resolve_ipv4("global-root-ca.chain-demos.digicert.com", ip, 0) > 0);

    /* Create the socket */
    socket_session_t client = socket_session_create();
    TEST_NOT_NULL(client);

    /* Enable TLS */
    socket_session_enable_tls_client(client, CERT_FILE);

    /* Connect to the server */
    TEST_EQUAL(socket_session_connect(client, ip, 443), SOCKET_STATE_CONNECTED);

    /* Get a message for the server from stdin */
    const char * request = "GET / HTTP/1.1\r\nHost: global-root-ca.chain-demos.digicert.com\r\nAccept-Language: en-us\r\nConnection: close\r\n\r\n";

    /* Send the message to the server */
    TEST_EQUAL(socket_session_write(client, request, strlen(request)), strlen(request));

    /* Read the server data into our buff array */
    TEST_TRUE(socket_session_buffer_until(client, condition, 1024, 1024 * 64));
    const char *expected_result = "HTTP/1.1 200 OK";
    TEST_TRUE(strncmp(client->buffer, expected_result, strlen(expected_result)) == 0);

    // Cleanup
    socket_session_shutdown(client);
    TEST_EQUAL(client->state, SOCKET_STATE_SHUTDOWN);
    socket_session_disconnect(client);
    socket_session_destroy(client);
    socket_session_cleanup();
    return 0;               /* Return reporting a success               */
}