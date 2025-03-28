#include "app/bt_agent.h"

#include <glib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "adapter.h"
#include "advertisement.h"
#include "agent.h"
#include "device.h"

const char A2DP_SRC_UUID[] = "0000110A-0000-1000-8000-00805F9B34FB";
const char A2DP_SNK_UUID[] = "0000110B-0000-1000-8000-00805F9B34FB";
const char A2DP_PROF_UUID[] = "0000110D-0000-1000-8000-00805F9B34FB";
const char AVRCP_PROF_UUID[] = "0000110E-0000-1000-8000-00805F9B34FB";

static pthread_t thread_id;
static bool stop_agent;
static bool initialized;

GMainLoop *loop = NULL;
Adapter *default_adapter = NULL;
Advertisement *advertisement = NULL;

void *agent_task(void *arg);

void on_central_state_changed(Adapter *adapter, Device *device)
{
    ConnectionState state = binc_device_get_connection_state(device);
    if (state == BINC_CONNECTED)
    {
        binc_adapter_stop_advertising(adapter, advertisement);
    }
    else if (state == BINC_DISCONNECTED)
    {
        binc_adapter_start_advertising(adapter, advertisement);
    }
}

void bt_agent_init()
{
    GDBusConnection *dbusconn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    loop = g_main_loop_new(NULL, false);

    default_adapter = binc_adapter_get_default(dbusconn);

    if (default_adapter != NULL)
    {
        if (!binc_adapter_get_powered_state(default_adapter))
        {
            binc_adapter_power_on(default_adapter);
        }

        binc_adapter_set_remote_central_cb(default_adapter, on_central_state_changed);

        GPtrArray *adv_service_uuids = g_ptr_array_new();
        g_ptr_array_add(adv_service_uuids, A2DP_SNK_UUID);
        g_ptr_array_add(adv_service_uuids, AVRCP_PROF_UUID);

        advertisement = binc_advertisement_create();
        binc_advertisement_set_local_name(advertisement, "CMPT433 Speaker");
        binc_advertisement_set_interval(advertisement, 500, 500);
        binc_advertisement_set_tx_power(advertisement, 5);
        binc_advertisement_set_services(advertisement, adv_service_uuids);
        binc_advertisement_set_appearance(advertisement, 0x0840);
        g_ptr_array_free(adv_service_uuids, TRUE);
        binc_adapter_start_advertising(default_adapter, advertisement);
    }

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