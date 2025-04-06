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

void bt_player_set_pause_sync(GDBusProxy *player_proxy){
    GError *error = NULL;

    if (g_dbus_proxy_call_sync(player_proxy,
                            "Pause",
                            NULL,
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            NULL,
                            &error) == NULL)
    {
        g_printerr("%s\n", error->message);
        // return false;
    } else {
        g_printerr("\nSuccess pause\n");
    }
}

void bt_player_set_play_sync(GDBusProxy *player_proxy){
    GError *error = NULL;

        if (g_dbus_proxy_call_sync(player_proxy,
                                    "Play",
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error) == NULL)
        {
        g_printerr("%s\n", error->message);
        // return false;
        } else {
            g_printerr("\nSuccess play\n");
        }
}


#include <string.h>
#include <stdio.h>

#define MAX_STRING_LENGTH 256

char current_artist[MAX_STRING_LENGTH];
char current_title[MAX_STRING_LENGTH];
char current_album[MAX_STRING_LENGTH];
char current_track_status[MAX_STRING_LENGTH];

void print_track_data(GDBusProxy *player_proxy) {

    GVariant *property = g_dbus_proxy_get_cached_property(player_proxy, "Track");
    gchar *str = g_variant_print(property, FALSE);

    gchar *title = NULL;
    gchar *track_number = NULL;
    gchar *num_tracks = NULL;
    gchar *duration = NULL;
    gchar *album = NULL;
    gchar *artist = NULL;
    gchar *status = NULL;

    // Parsing 'Title'
    gchar *title_start = strstr(str, "'Title': <'");
    if (title_start != NULL) {
        title_start += 11;  // Skip "'Title': <'"
        gchar *title_end = strstr(title_start, "'>");
        if (title_end != NULL) {
            gint title_length = title_end - title_start;
            title = g_strndup(title_start, title_length);

            if (title_length < MAX_STRING_LENGTH) {
                strncpy(current_title, title_start, title_length);
                current_title[title_length] = '\0';
            } else {
                // If string is too long, just copy the maximum allowed length
                strncpy(current_title, title_start, MAX_STRING_LENGTH - 1);
                current_title[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    // Parsing 'TrackNumber'
    gchar *track_num_start = strstr(str, "'TrackNumber': <");
    if (track_num_start != NULL) {
        track_num_start += 23;
        gchar *track_num_end = strstr(track_num_start, ">");
        if (track_num_end != NULL) {
            gint track_num_length = track_num_end - track_num_start;
            track_number = g_strndup(track_num_start, track_num_length);
        }
    }
    int tarck_num_test = (int) g_ascii_strtoull(track_number, NULL, 10);

    // Parsing 'NumberOfTracks'
    gchar *num_tracks_start = strstr(str, "'NumberOfTracks': <");
    if (num_tracks_start != NULL) {
        num_tracks_start += 26;
        gchar *num_tracks_end = strstr(num_tracks_start, ">");
        if (num_tracks_end != NULL) {
            gint num_tracks_length = num_tracks_end - num_tracks_start;
            num_tracks = g_strndup(num_tracks_start, num_tracks_length);
        }
    }
    int num_tracks_test = (int) g_ascii_strtoull(num_tracks, NULL, 10);

    // Parsing 'Duration'
    gchar *duration_start = strstr(str, "'Duration': <uint32 ");
    if (duration_start != NULL) {
        duration_start += 19; 
        gchar *duration_end = strstr(duration_start, ">");
        if (duration_end != NULL) {
            gint duration_length = duration_end - duration_start;
            duration = g_strndup(duration_start, duration_length);
        }
    }
    int dur_test = (int) g_ascii_strtoull(duration, NULL, 10);

    // Parsing 'Album'
    gchar *album_start = strstr(str, "'Album': <'");
    if (album_start != NULL) {
        album_start += 11;
        gchar *album_end = strstr(album_start, "'>");
        if (album_end != NULL) {
            gint album_length = album_end - album_start;
            album = g_strndup(album_start, album_length);

            if (album_length < MAX_STRING_LENGTH) {
                strncpy(current_album, album_start, album_length);
                current_album[album_length] = '\0';
            } else {
                strncpy(current_album, album_start, MAX_STRING_LENGTH - 1);
                current_album[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    // Parsing 'Artist'
    gchar *artist_start = strstr(str, "'Artist': <'");
    if (artist_start != NULL) {
        artist_start += 12;
        gchar *artist_end = strstr(artist_start, "'>");
        if (artist_end != NULL) {
            gint artist_length = artist_end - artist_start;
            artist = g_strndup(artist_start, artist_length);

            if (artist_length < MAX_STRING_LENGTH) {
                strncpy(current_artist, artist_start, artist_length);
                current_artist[artist_length] = '\0';
            } else {
                strncpy(current_artist, artist_start, MAX_STRING_LENGTH - 1);
                current_artist[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    property = g_dbus_proxy_get_cached_property(player_proxy, "Status");
    gchar *str_other = g_variant_print(property, FALSE);

    gchar *status_start = strstr(str_other, "'");
    if (status_start != NULL) {
        status_start += 1;
        gchar *status_end = strstr(status_start, "'");
        if (status_end != NULL) {
            gint status_length = status_end - status_start;
            status = g_strndup(status_start, status_length);

            if (status_length < MAX_STRING_LENGTH) {
                strncpy(current_track_status, status_start, status_length);
                current_track_status[status_length] = '\0';
            } else {
                strncpy(current_track_status, status_start, MAX_STRING_LENGTH - 1);
                current_track_status[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    property = g_dbus_proxy_get_cached_property(player_proxy, "Position");
    str_other = g_variant_print(property, FALSE);
    int curr_pos_test = (int) g_ascii_strtoull(str_other, NULL, 10);

    
    if (current_track_status[0] != '\0') {
        printf("Status: '%s'\n", current_track_status);
    } else {
        printf("Artist: Unknown\n"); }
    if (current_title[0] != '\0') {
        printf("Artist: '%s'\n", current_title);
    } else {
        printf("Artist: Unknown\n"); }

    printf("Track Number: %d\n", tarck_num_test);
    printf("Number of Tracks: %d\n", num_tracks_test);
    printf("Duration: %d\n", dur_test ? dur_test : -1);
    printf("Current Position: %d\n", curr_pos_test ? curr_pos_test : -1);

    if (current_album[0] != '\0') {
        printf("Album: '%s'\n", current_album);
    } else {
        printf("Album: Unknown\n"); }
    if (current_artist[0] != '\0') {
        printf("Artist: '%s'\n", current_artist);
    } else {
        printf("Artist: Unknown\n"); }
    
    // Clean up memory
    g_free(str);
    g_free(title);
    g_free(track_number);
    g_free(num_tracks);
    g_free(duration);
    g_free(album);
    g_free(artist);
    g_free(status);
    g_free(str_other);
}

bool is_paused_status(){
    if(strcmp(current_track_status, "paused") == 0){
        return true;
    } else {
        return false;
    }
}