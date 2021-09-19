#include "socket_session_pool.h"
#include <memory.h>

socket_session_pool_t socket_session_pool_init()
{
    socket_session_pool_t result = (struct socket_session_pool_struct*)malloc(sizeof(struct socket_session_pool_struct));
    result->sessions = NULL;
    result->num_sessions = 0;
}

void socket_session_pool_add(socket_session_pool_t pool, socket_session_t session)
{
    if(pool->num_sessions == 0) {
        pool->sessions = (socket_session_t*)malloc(sizeof(socket_session_t));
        pool->sessions[0] = session;
        pool->num_sessions = 1;
    } else {
        // Scan for existing entry
        for(size_t i = 0; i < pool->num_sessions; i++) {
            if(pool->sessions[i] == session) return;
        }

        // Resize the array
        pool->sessions = realloc(pool->sessions, (pool->num_sessions + 1) * sizeof(socket_session_t));

        // Add the session
        pool->sessions[pool->num_sessions] = session;
    }
}

void socket_session_pool_remove(socket_session_pool_t pool, socket_session_t session)
{
    size_t i = 0;
    while(i < pool->num_sessions)
    {
        if(pool->sessions[i] == session) {
            const size_t sessions_remaining = pool->num_sessions - i - 1;
            if(sessions_remaining > 0) {
                memmove(&pool->sessions[i], &pool->sessions[i + 1], sessions_remaining * sizeof(socket_session_t));
            }
            pool->num_sessions--;
        } else {
            i++;
        }
    }
    pool->sessions = realloc(pool->sessions, (pool->num_sessions) * sizeof(socket_session_t));
}

void socket_session_pool_clear(socket_session_pool_t pool)
{
    free(pool->sessions);
    pool->sessions = NULL;
    pool->num_sessions = 0;
}

void socket_session_pool_destroy(socket_session_pool_t pool)
{
    socket_session_pool_clear(pool);
    free(pool);
}

socket_session_pool_t socket_session_pool_poll(socket_session_pool_t pool);