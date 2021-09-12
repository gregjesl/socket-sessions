#include "socket_session.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <WS2tcpip.h>
#define poll(a,b,c) WSAPoll(a, b, c)
static bool winsock_initialized = false;
static WSADATA wsaData = { 0 };
#define inet_pton(a, b, c) InetPton(a,b,c)
#define SHUT_WR SD_SEND
typedef int ssize_t;
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#endif // WIN32

#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)

#define SESSION_RECEIVE_CAPABLE(session) (session->state == SOCKET_STATE_CONNECTED || session->state == SOCKET_STATE_PEER_CLOSED || session->state == SOCKET_STATE_SHUTDOWN)

#ifdef WIN32
void winsock_init()
{
	if (!winsock_initialized) {
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
			perror("Could not initialize WSA");
			exit(1);
		}
		winsock_initialized = true;
	}
}
#endif

SOCKET __init_socket()
{
#ifdef WIN32
	winsock_init();
    SOCKET sock = INVALID_SOCKET;
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        perror("Socket creation error");
#ifdef WIN32
		const int error = WSAGetLastError();
		printf("%i\n", error);
#endif // WIN32
		exit(1);
    }
#else
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket creation error");
		exit(1);
    }
#endif // WIN32
    return sock;
}

/*
void monitor_thread(void *arg)
{
    const int poll_period_ms = 10;
    socket_session_t session = (socket_session_t)arg;

    do
    {
        __read_cycle(session, poll_period_ms);

        // Check for a timeout
        if(
            session->socket->timeout > 0.0f
            &&
            session->socket->state == SOCKET_STATE_CONNECTED
            &&
            (float)session->socket->last_activity / 1000.0f > session->socket->timeout
        ) {
            session->socket->state = SOCKET_STATE_TIMEOUT;
        }

    } while (session->socket->state == SOCKET_STATE_CONNECTED);

    switch (session->socket->state)
    {
    case SOCKET_STATE_CLOSED:
    case SOCKET_STATE_PEER_CLOSED:
        while(session->socket->state == SOCKET_STATE_PEER_CLOSED) {
            __read_cycle(session, 0);
        }
        if(session->hangup_callback != NULL) {
            session->hangup_callback(session->socket);
        }
        break;
    case SOCKET_STATE_TIMEOUT:
        if(session->timeout_callback != NULL) {
            session->timeout_callback(session->socket);
        }
        break;
    case SOCKET_STATE_SHUTDOWN:
        // Do nothing
        break;
    default:
    case SOCKET_STATE_ERROR:
        session->socket->state = SOCKET_STATE_ERROR;
        if(session->error_callback != NULL) {
            session->error_callback(session->socket);
        }
        break;
    }

    // Close the socket
    socket_wrapper_close(session->socket);
    assert(session->socket->state == SOCKET_STATE_CLOSED);

    // Notify 
    if(session->finalize_callback != NULL) {
        session->finalize_callback(session->socket);
    }

    // Wait for the signal to shut down
    socket_wrapper_destroy(session->socket);
    free(session);
}
*/

socket_session_t socket_session_init(SOCKET id)
{
	// Create the structure
    socket_session_t result = (socket_session_t)malloc(sizeof(struct socket_session_struct));

	// Set the socket ID
	result->id = id;

	// Set the thread to null
	result->thread = NULL;

	// Initialize the mutex
	result->mutex = macrothread_mutex_init();

	// Initialize the condition
	result->condition = macrothread_condition_init();

	// Set the state
	result->state = SOCKET_STATE_CLOSED;

    // Initialize the buffer
    result->buffer = NULL;
    result->buffer_len = 0;

    // Return the result
    return result;
}

socket_session_t socket_session_create()
{
    return socket_session_init(__init_socket());
}

size_t socket_session_resolve_ipv4(const char *fqdn, socket_session_ipv4_address result, size_t skip)
{
    size_t num_results = 0;
    struct addrinfo      hints;  //prefered addr type(connection)
    struct addrinfo  *   list = NULL;   //list of addr structs
    struct addrinfo  *   ptr = NULL;   // Iterator

    #ifdef WIN32
    winsock_init();
    #endif

    memset(result, 0, sizeof(socket_session_ipv4_address));

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags                = 0;
    hints.ai_family               = AF_INET;
    hints.ai_socktype             = SOCK_RAW;
    hints.ai_protocol             = IPPROTO_ICMP;

    if (getaddrinfo(fqdn, 0, &hints, &list) < 0) {
        return 0;
    }

    // Pull the first entry
    if (list != NULL) {
        
    }

    for(ptr=list; ptr != NULL; ptr=ptr->ai_next) {
        if(num_results == skip) {
            getnameinfo(ptr->ai_addr, ptr->ai_addrlen, result, sizeof(socket_session_ipv4_address), NULL, 0, NI_NUMERICHOST);
        }
        num_results++;
    }

    freeaddrinfo(list);
    return num_results;
}

socket_session_state_t socket_session_connect(socket_session_t session, const char *address, const int port)
{
    struct sockaddr_in serv_addr;

    // Validate the inputs
    if(session == NULL || address == NULL) return SOCKET_STATE_ERROR;

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons((unsigned short)port);
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0) return SOCKET_STATE_ERROR;
   
    unsigned int retry_count = 0;
    while(retry_count < 3) {
        if (connect(session->id, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            if(retry_count > 2) {
                perror("Error encountered in connect()");
                return SOCKET_STATE_ERROR;
            } else {
                retry_count++;
                macrothread_delay(10);
            }
        } else {
            break;
        }
    }

    session->state = SOCKET_STATE_CONNECTED;

    return SOCKET_STATE_CONNECTED;
}

/*
void socket_session_start(socket_session_t session)
{
    // Start monitoring the socket
    session->thread = macrothread_handle_init();
    session->thread->detached = true;
    macrothread_start_thread(session->thread, monitor_thread, session);
}
*/

size_t socket_session_read(socket_session_t session, char *buffer, size_t max_bytes)
{
	struct pollfd pfd;
	ssize_t bytes_read = 0;
	size_t total_bytes_read = 0;

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = session->id;
	pfd.events = POLLIN;

	while (
		(session->state == SOCKET_STATE_CONNECTED || session->state == SOCKET_STATE_PEER_CLOSED)
		&&
		total_bytes_read < max_bytes
		&&
		poll(&pfd, 1, 0) > 0
		)
	{
		if (pfd.revents & POLLIN) {
#ifdef WIN32
			bytes_read = recv(session->id, buffer + total_bytes_read, (int)(max_bytes - total_bytes_read), 0);
#else
			bytes_read = read(session->id, buffer + total_bytes_read, max_bytes - total_bytes_read);
#endif

			if (bytes_read < 0) {
				session->state = SOCKET_STATE_ERROR;
				goto exit;
			}

			total_bytes_read += bytes_read;
		}

		if ((pfd.revents & POLLHUP) || (pfd.revents & POLLNVAL) || bytes_read == 0) {
			session->state = (bytes_read > 0) ? SOCKET_STATE_PEER_CLOSED : SOCKET_STATE_CLOSED;
			break;
		}

		if (pfd.revents & POLLERR) {
			session->state = SOCKET_STATE_ERROR;
			goto exit;
		}
	}
	
	exit:
	return total_bytes_read;
}

size_t socket_session_write(socket_session_t session, const char *buffer, size_t length)
{
	ssize_t bytes_written = 0;
	const char *write_index = buffer;
	size_t bytes_to_write = length;

	if (session->state != SOCKET_STATE_CONNECTED) {
		goto exit;
	}

	while (bytes_to_write > 0) {
#ifdef WIN32
		bytes_written = send(session->id, write_index, (int)bytes_to_write, 0);
#else
		bytes_written = write(session->id, write_index, bytes_to_write);
#endif

		if (bytes_written < 0) {
#ifdef WIN32
			if (bytes_written == WSAETIMEDOUT ||
				bytes_written == WSAECONNRESET
				) {
				session->state = SOCKET_STATE_PEER_CLOSED;
				goto exit;
			}
			else {
				session->state = SOCKET_STATE_ERROR;
				goto exit;
			}
#else
			if (errno == EPIPE) {
				session->state = SOCKET_STATE_PEER_CLOSED;
				goto exit;
			}
			else {
				session->state = SOCKET_STATE_ERROR;
				goto exit;
			}
#endif
		}

		bytes_to_write -= bytes_written;
		write_index += bytes_written;
	}

	exit:
	return bytes_written;
}

socket_session_state_t socket_session_wait(socket_session_t session)
{
    struct pollfd pfd;

    // Verify the input
    if(session == NULL) return SOCKET_STATE_ERROR;

    // Check the entry conditions
    if(!SESSION_RECEIVE_CAPABLE(session)) return session->state;

    // Set up poll
    restart:
	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = session->id;
	pfd.events = POLLIN;

    // Loop until data
    while(poll(&pfd, 1, -1) == 0) {
        macrothread_delay(1);
    }

    if(pfd.revents & POLLERR) {
        session->state = SOCKET_STATE_ERROR;
        return session->state;
    } else if(pfd.revents & POLLHUP) {
        session->state = SOCKET_STATE_PEER_CLOSED;
        return session->state;
    } else if(pfd.revents & POLLNVAL) {
        session->state = SOCKET_STATE_ERROR;
        return session->state;
    } else if(pfd.revents & POLLIN) {
        return session->state;
    } else {
        goto restart;
    }
}

void __resize_buffer(socket_session_t session, size_t size)
{
    // Check for a valid session
    if(session == NULL) return;

    // Resize the buffer
    session->buffer = (char*)realloc(session->buffer, size);
    session->buffer_len = size;
}

size_t socket_session_buffer(socket_session_t session, size_t min_bytes, size_t max_bytes)
{
    // Check for a valid session
    if(session == NULL) return 0;

    // Check for a sufficently-populated buffer
    if(session->buffer_len >= min_bytes) return session->buffer_len;

    // Expand the buffer to the max size
    session->buffer = (char*)realloc(session->buffer, max_bytes);

    // Loop until min bytes is met
    while(SESSION_RECEIVE_CAPABLE(session) && session->buffer_len < min_bytes)
    {
        // Read the bytes
        const size_t bytes_read = socket_session_read(session, session->buffer + session->buffer_len, max_bytes - session->buffer_len);

        // Check for a closed peer
        if(bytes_read == 0 && (session->state == SOCKET_STATE_PEER_CLOSED || session->state == SOCKET_STATE_CLOSED)) goto exit;

        // Move the buffer counter
        session->buffer_len += bytes_read;
    }

    // Exit label
    exit:

    // Condense the buffer to the new size
    __resize_buffer(session, session->buffer_len);

    // Return the new buffer length
    return session->buffer_len;
}

bool socket_session_buffer_until(socket_session_t session, socket_buffer_condition condition, size_t increment, size_t max_bytes)
{
    // Check for valid inputs
    if(session == NULL || max_bytes == 0) return 0;

    // Seed the initial buffer if needed
    if(session->buffer == NULL || session->buffer_len == 0) {
        const size_t bytes_read = socket_session_buffer(session, increment, max_bytes);
        if(bytes_read < increment) return bytes_read;
    }

    // At this point, the buffer should be populated with at least increment bytes
    assert(session->buffer != NULL);
    assert(session->buffer_len >= increment);

    // Check for a passing initial condition
    while(!condition(session->buffer, session->buffer_len) && session->buffer_len < max_bytes) {
        // Determine the minimum number of bytes, which should be less than or equal to the maximum number of bytes
        const size_t min_bytes = session->buffer_len + increment > max_bytes ? max_bytes : session->buffer_len + increment;

        // Buffer the bytes
        const size_t bytes_read = socket_session_buffer(session, min_bytes, max_bytes);
        
        // Check for an error
        if(bytes_read < min_bytes) {
            return bytes_read;
        }
    }

    return condition(session->buffer, session->buffer_len);
}

size_t socket_session_flush(socket_session_t session, size_t bytes_to_flush)
{
    if(bytes_to_flush < session->buffer_len) 
    {
        memmove(session->buffer, session->buffer + bytes_to_flush, session->buffer_len - bytes_to_flush);
        session->buffer_len -= bytes_to_flush;
        session->buffer = (char*)realloc(session->buffer, session->buffer_len);
        return session->buffer_len;
    }
    else 
    {
        free(session->buffer);
        session->buffer = NULL;
        session->buffer_len = 0;
        return 0;
    }
}

void socket_session_shutdown(socket_session_t session)
{
	if (session->state == SOCKET_STATE_CONNECTED || session->state == SOCKET_STATE_PEER_CLOSED) {
		session->state = SOCKET_STATE_SHUTDOWN;
		shutdown(session->id, SHUT_WR);
	}
}

void socket_session_disconnect(socket_session_t session)
{
#ifdef WIN32
	closesocket(session->id);
#else
	close(session->id);
#endif
	session->state = SOCKET_STATE_CLOSED;
}

void socket_session_destroy(socket_session_t session)
{
    macrothread_handle_destroy(session->thread);
    macrothread_mutex_destroy(session->mutex);
    macrothread_condition_destroy(session->condition);
    if(session->buffer != NULL) free(session->buffer);
    free(session);
}