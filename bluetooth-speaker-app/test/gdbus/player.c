#include "player.h"

#include <gio/gio.h>

#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"
#define BLUEZ_MEDIA_TRANSPORT_INTERFACE "org.bluez.MediaTransport1"
#define DBUS_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"

typedef struct
{
    bt_player_cmd_cb cb;
    void *user_data;
} cmd_cb_data_t;

typedef struct
{
    bt_player_prop_cb cb;
    void *user_data;
} prop_cb_data_t;

static struct
{
    GDBusProxy *player_proxy;
    GDBusProxy *transport_proxy;

    cmd_cb_data_t cmd_cb_data[BT_PLAYER_CMD_cnt];
    prop_cb_data_t prop_cb_data[BT_PLAYER_PROP_cnt];
} data;

static const gchar *COMMAND_STRINGS[BT_PLAYER_CMD_cnt] = {
    [BT_PLAYER_CMD_PAUSE] = "Pause",
    [BT_PLAYER_CMD_PLAY] = "Play",
    [BT_PLAYER_CMD_STOP] = "Stop",
    [BT_PLAYER_CMD_NEXT] = "Next",
    [BT_PLAYER_CMD_PREV] = "Previous",
};

static const gchar *SHUFFLE_STRINGS[BT_PLAYER_SHUFFLE_cnt] = {
    [BT_PLAYER_SHUFFLE_OFF] = "off",
    [BT_PLAYER_SHUFFLE_ALL_TRACKS] = "alltracks",
};

static const gchar *REPEAT_STRINGS[BT_PLAYER_REPEAT_cnt] = {
    [BT_PLAYER_REPEAT_OFF] = "off",
    [BT_PLAYER_REPEAT_SINGLE_TRACK] = "singletrack",
    [BT_PLAYER_REPEAT_ALL_TRACKS] = "alltracks",
};

typedef struct
{
    GDBusProxy **proxy;
    gchar *prop_name;
} prop_entry_t;

static prop_entry_t prop_entries[BT_PLAYER_PROP_cnt] = {
    [BT_PLAYER_PROP_PLAYBACK_STATUS] = {
        .proxy = &data.player_proxy,
        .prop_name = "Status",
    },
    [BT_PLAYER_PROP_SHUFFLE_MODE] = {
        .proxy = &data.player_proxy,
        .prop_name = "Shuffle",
    },
    [BT_PLAYER_PROP_REPEAT_MODE] = {
        .proxy = &data.player_proxy,
        .prop_name = "Repeat",
    },
    [BT_PLAYER_PROP_PLAYBACK_POSITION] = {
        .proxy = &data.player_proxy,
        .prop_name = "Position",
    },
    [BT_PLAYER_PROP_TRACK] = {
        .proxy = &data.player_proxy,
        .prop_name = "Track",
    },
    [BT_PLAYER_PROP_VOLUME] = {
        .proxy = &data.transport_proxy,
        .prop_name = "Volume",
    },
};

static bool proxy_set_property(GDBusProxy *proxy,
                               const gchar *property_name,
                               GVariant *value,
                               GError **error)
{
    GVariant *param = g_variant_new("(ssv)",
                                    g_dbus_proxy_get_interface_name(proxy),
                                    property_name, value);

    return g_dbus_proxy_call_sync(proxy,
                                  DBUS_PROPERTIES_INTERFACE ".Set",
                                  param,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  error) != NULL;
}

static GVariant *proxy_get_property(GDBusProxy *proxy,
                                    const gchar *property_name,
                                    GError **error)
{
    GVariant *param = g_variant_new("(ss)",
                                    g_dbus_proxy_get_interface_name(proxy),
                                    property_name);

    return g_dbus_proxy_call_sync(proxy,
                                  DBUS_PROPERTIES_INTERFACE ".Get",
                                  param,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  error);
}

bool bt_player_send_command(bt_player_cmd_e command)
{
    GError *error;

    if (g_dbus_proxy_call_sync(data.player_proxy,
                               COMMAND_STRINGS[command],
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               &error) == NULL)
    {
        g_printerr("%s\n", error->message);
        return false;
    }

    return true;
}

static void send_command_async_done(GObject *source_object,
                                    GAsyncResult *res,
                                    gpointer user_data)
{
    cmd_cb_data_t *cb_data;
    GError *error = NULL;
    bool success;

    cb_data = user_data;

    success = NULL != g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);
    if (!success)
        g_printerr("%s\n", error->message);

    if (cb_data->cb)
        cb_data->cb(success, cb_data->user_data);
}

void bt_player_send_command_async(bt_player_cmd_e command,
                                  bt_player_cmd_cb callback,
                                  void *user_data)
{
    data.cmd_cb_data[command].cb = callback;
    data.cmd_cb_data[command].user_data = user_data;

    g_dbus_proxy_call(data.player_proxy,
                      COMMAND_STRINGS[command],
                      NULL,
                      G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      send_command_async_done,
                      &data.cmd_cb_data[command]);
}

bool bt_player_get_property(bt_player_prop_e property, void *retval)
{
    (void)property;
    (void)retval;

    return false;
}

bool bt_player_set_property(bt_player_prop_e property, void *arg)
{
    prop_entry_t *prop_entry;
    GVariant *value;
    GError *error;

    prop_entry = &prop_entries[property];

    switch (property)
    {
    case BT_PLAYER_PROP_SHUFFLE_MODE:
        value = g_variant_new_string(
            SHUFFLE_STRINGS[*(bt_player_shuffle_e *)arg]);
        break;
    case BT_PLAYER_PROP_REPEAT_MODE:
        value = g_variant_new_string(
            REPEAT_STRINGS[*(bt_player_repeat_e *)arg]);
        break;
    case BT_PLAYER_PROP_VOLUME:
        value = g_variant_new_uint16(*(uint8_t *)arg);
        break;
    default:
        g_printerr("Property '%s' is not writeable\n",
                   prop_entry->prop_name);
        break;
    }

    if (!*prop_entry->proxy)
    {
        g_printerr("Proxy for property '%s' is unavailable\n",
                   prop_entry->prop_name);
        return false;
    }

    error = NULL;
    if (!proxy_get_property(*prop_entry->proxy,
                            prop_entry->prop_name,
                            &error))
    {
        g_printerr("%s\n", error->message);
        return false;
    }

    error = NULL;
    if (!proxy_set_property(*prop_entry->proxy,
                            prop_entry->prop_name,
                            value,
                            &error))
    {
        g_printerr("%s\n", error->message);
        return false;
    }

    return true;
}

void bt_player_set_property_changed_cb(bt_player_prop_e property,
                                       bt_player_prop_cb callback,
                                       void *user_data)
{
    (void)property;
    (void)callback;
    (void)user_data;

    return;
}