#include "socket_manager.h"
#include "socket_session.h"
#include <string.h>

void socket_manager_init_thread(void *arg)
{
    // Do nothing
}
socket_manager_t socket_manager_init()
{
    socket_manager_t result = (socket_manager_t)malloc(sizeof(struct socket_manager_struct));
    result->handle = macrothread_handle_init();
    result->mutex = macrothread_mutex_init();
    result->confirmation = macrothread_condition_init();

    // Start the empty thread
    macrothread_start_thread(result->handle, socket_manager_init_thread, NULL);

    return result;
}

void socket_manager_destroy(socket_manager_t manager)
{
    // Wait until the last thread finishes
    macrothread_join(manager->handle);

    // Cleanup
    macrothread_handle_destroy(manager->handle);
    macrothread_mutex_destroy(manager->mutex);
    macrothread_condition_destroy(manager->confirmation);
    free(manager);
}