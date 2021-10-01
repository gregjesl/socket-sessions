#include "socket_session.h"

bool wolfssl_init = false;

void socket_session_enable_tls_client(socket_session_t session, const char *ca_cert)
{
    if(!wolfssl_init) {
        if(wolfSSL_Init() != WOLFSSL_SUCCESS) {
            fprintf(stderr, "ERROR: Failed to initialize the library\n");
            return;
        }
        wolfssl_init = true;
    }

    // Create the structure
    session->tls = (socket_session_tls_t)malloc(sizeof(struct socket_session_tls_struct));

    // Intialize the context
    if((session->tls->ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        free(session->tls);
        session->tls = NULL;
        return;
    }

    if (wolfSSL_CTX_load_verify_locations(session->tls->ctx, ca_cert, NULL)
        != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                ca_cert);
        wolfSSL_CTX_free(session->tls->ctx);
        free(session->tls);
        session->tls = NULL;
        return;
    }

    if ((session->tls->ssl = wolfSSL_new(session->tls->ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        wolfSSL_CTX_free(session->tls->ctx);
        free(session->tls);
        session->tls = NULL;
        return;
    }
}

bool socket_session_tls_enabled(socket_session_t session)
{
    return session->tls != NULL;
}

void socket_session_cleanup()
{
    if(wolfssl_init) {
        wolfSSL_Cleanup();
    }
    #ifdef WIN32
    WSACleanup();
    #endif
}