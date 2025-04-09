#include "hal/bt_agent.h"

#include <glib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "adapter.h"
#include "advertisement.h"
#include "agent.h"
#include "device.h"
#include "application.h"

#define A2DP_SRC_UUID "0000110a-0000-1000-8000-00805f9b34fb"
#define A2DP_SNK_UUID "0000110b-0000-1000-8000-00805f9b34fb"
#define A2DP_PROF_UUID "0000110d-0000-1000-8000-00805f9b34fb"
#define AVCRP_PROF_UUID "0000110e-0000-1000-8000-00805f9b34fb"

GDBusConnection *dbusconn;
GMainLoop *main_loop = NULL;
Adapter *default_adapter = NULL;
Application *app = NULL;
Agent *agent = NULL;

pthread_t agent_thread_id;

static void *agent_routine(void *arg);

void on_central_state_changed(Adapter *adapter, Device *device)
{
    (void)adapter;

    ConnectionState state = binc_device_get_connection_state(device);
    if (state == BINC_CONNECTED)
    {
        binc_adapter_discoverable_off(adapter);
    }
    else if (state == BINC_DISCONNECTED)
    {
        binc_adapter_discoverable_on(adapter);
    }
}

gboolean on_request_authorization(Device *device)
{
    (void)device;
    return TRUE;
}

void bt_agent_init()
{
    dbusconn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    default_adapter = binc_adapter_get_default(dbusconn);

    if (default_adapter != NULL)
    {
        if (!binc_adapter_get_powered_state(default_adapter))
        {
            binc_adapter_power_on(default_adapter);
        }

        binc_adapter_set_remote_central_cb(default_adapter, on_central_state_changed);

        GList *devices = binc_adapter_get_devices(default_adapter);
        for (GList *iter = devices; iter; iter = iter->next)
        {
            Device *dev = iter->data;
            binc_device_connect(dev);
        }

        binc_adapter_discoverable_on(default_adapter);

        agent = binc_agent_create(default_adapter, "/speaker/agent", NO_INPUT_NO_OUTPUT);
        binc_agent_set_request_authorization_cb(agent, &on_request_authorization);

        app = binc_create_application(default_adapter);
        binc_application_add_service(app, A2DP_SNK_UUID);
        binc_application_add_service(app, AVCRP_PROF_UUID);
    }

    pthread_create(&agent_thread_id, NULL, agent_routine, NULL);
}

void bt_agent_cleanup()
{
    GList *devices = binc_adapter_get_devices(default_adapter);
    for (GList *iter = devices; iter; iter = iter->next)
    {
        Device *dev = iter->data;
        if (dev)
        {
            binc_device_disconnect(dev);
        }
    }
    g_list_free(devices);

    if (app != NULL)
    {
        binc_adapter_unregister_application(default_adapter, app);
        binc_application_free(app);
        app = NULL;
    }

    if (agent != NULL)
    {
        binc_agent_free(agent);
        agent = NULL;
    }

    if (default_adapter != NULL)
    {
        binc_adapter_free(default_adapter);
        default_adapter = NULL;
    }

    g_main_loop_quit(main_loop);

    pthread_join(agent_thread_id, NULL);

    g_dbus_connection_close_sync(dbusconn, NULL, NULL);
    g_object_unref(dbusconn);
}

static void *agent_routine(void *arg)
{
    (void)arg;

    GMainContext *main_context = g_main_context_get_thread_default();
    main_loop = g_main_loop_new(main_context, FALSE);

    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);

    return NULL;
}