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

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include "wolfssl/options.h"
#include "wolfssl/ssl.h"

/* Socket session */
#include "socket_session.h"

#define CERT_FILE "./cert.pem"
#include "test.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    int                sockfd;
    struct sockaddr_in servAddr;
    char               buff[1024 * 64];
    socket_session_ipv4_address ip;               
    size_t             len;
    int                ret;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(443);       /* on DEFAULT_PORT */

    /* Get the IP address */
    TEST_TRUE(socket_session_resolve_ipv4("global-root-ca.chain-demos.digicert.com", ip, 0) > 0);

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, ip, &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        ret = -1;
        goto end;
    }

    /* Connect to the server */
    if ((ret = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)))
         == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto end;
    }

    /*---------------------------------*/
    /* Start of security */
    /*---------------------------------*/
    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto socket_cleanup;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto socket_cleanup;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL))
         != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        goto ctx_cleanup;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto cleanup;
    }

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto cleanup;
    }

    /* Get a message for the server from stdin */
    strcpy(buff, "GET / HTTP/1.1\r\nHost: global-root-ca.chain-demos.digicert.com\r\nAccept-Language: en-us\r\nConnection: close\r\n\r\n");
    len = strnlen(buff, sizeof(buff));

    /* Send the message to the server */
    if ((ret = wolfSSL_write(ssl, buff, len)) != (int)len) {
        fprintf(stderr, "ERROR: failed to write entire message\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }

    /* Print to stdout any data the server sends */
    const char *expected_result = "HTTP/1.1 200 OK";
    TEST_TRUE(strncmp(buff, expected_result, strlen(expected_result)) == 0);

    /* Bidirectional shutdown */
    while (wolfSSL_shutdown(ssl) == SSL_SHUTDOWN_NOT_DONE) {
        macrothread_delay(1);
    }

    ret = 0;

    /* Cleanup and return */
cleanup:
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
end:
    return ret;               /* Return reporting a success               */
}