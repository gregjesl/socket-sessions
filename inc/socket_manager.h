#ifndef SOCKET_SESSIONS_SOCKET_MANAGER_H
#define SOCKET_SESSIONS_SOCKET_MANAGER_H

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 
#endif // !WIN32_LEAN_AND_MEAN
#endif // WIN32

#include "macrothreading_thread.h"
#include "macrothreading_mutex.h"
#include "macrothreading_condition.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct socket_manager_struct
{
    macrothread_handle_t handle;
    void* incoming_session;
    macrothread_mutex_t mutex;
    macrothread_condition_t confirmation;
} *socket_manager_t;

socket_manager_t socket_manager_init();
void socket_manager_destroy(socket_manager_t manager);

#endif