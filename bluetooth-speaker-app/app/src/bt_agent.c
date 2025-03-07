#include "app/bt_agent.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_t thread_id;
static bool stop_agent;
static bool initialized;

void *agent_task(void *arg);

void bt_agent_init()
{
    stop_agent = false;
    if (pthread_create(&thread_id, NULL, agent_task, NULL) < 0)
    {
        perror(__func__);
        exit(EXIT_FAILURE);
    }

    initialized = true;
}

void bt_agent_cleanup()
{

    stop_agent = true;
    pthread_join(thread_id, NULL);
}


void *agent_task(void *arg)
{
    (void)arg;


    return NULL;
}