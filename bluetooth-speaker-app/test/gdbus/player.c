#include "player.h"
#include "dbus.h"

#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"
#define BLUEZ_MEDIA_TRANSPORT_INTERFACE "org.bluez.MediaTransport1"

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
    dbus_proxy_watch_t *proxy_watch;
    GDBusProxy *player_proxy;
    GDBusProxy *transport_proxy;
    gulong player_handler_id;
    gulong transport_handler_id;
    pthread_mutex_t proxy_mtx;

    prop_cb_data_t prop_cb_data[BT_PLAYER_PROP_cnt];
} data = {0};

static struct
{
    bt_player_status_e status;
    bt_player_shuffle_e shuffle;
    bt_player_repeat_e repeat;
    uint32_t position;
    bt_player_track_info_t track_info;
    uint8_t volume;
} props = {0};

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
    void *prop_val;
    bool (*update_val_fn)(GVariant *);
    pthread_mutex_t mtx;
} prop_entry_t;

static bool prop_update_status(GVariant *val);
static bool prop_update_shuffle(GVariant *val);
static bool prop_update_repeat(GVariant *val);
static bool prop_update_position(GVariant *val);
static bool prop_update_track(GVariant *val);
static bool prop_update_volume(GVariant *val);

static prop_entry_t prop_entries[BT_PLAYER_PROP_cnt] = {
    [BT_PLAYER_PROP_PLAYBACK_STATUS] = {
        .proxy = &data.player_proxy,
        .prop_name = "Status",
        .prop_val = &props.status,
        .update_val_fn = prop_update_status,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
    [BT_PLAYER_PROP_SHUFFLE_MODE] = {
        .proxy = &data.player_proxy,
        .prop_name = "Shuffle",
        .prop_val = &props.shuffle,
        .update_val_fn = prop_update_shuffle,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
    [BT_PLAYER_PROP_REPEAT_MODE] = {
        .proxy = &data.player_proxy,
        .prop_name = "Repeat",
        .prop_val = &props.repeat,
        .update_val_fn = prop_update_repeat,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
    [BT_PLAYER_PROP_PLAYBACK_POSITION] = {
        .proxy = &data.player_proxy,
        .prop_name = "Position",
        .prop_val = &props.position,
        .update_val_fn = prop_update_position,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
    [BT_PLAYER_PROP_TRACK] = {
        .proxy = &data.player_proxy,
        .prop_name = "Track",
        .prop_val = &props.track_info,
        .update_val_fn = prop_update_track,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
    [BT_PLAYER_PROP_VOLUME] = {
        .proxy = &data.transport_proxy,
        .prop_name = "Volume",
        .prop_val = &props.volume,
        .update_val_fn = prop_update_volume,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
    },
};

static bool proxy_filter(GDBusProxy *proxy);
static void proxy_changed_cb(GDBusProxy *proxy,
                             GVariant *changed_properties,
                             const gchar *const *invalidated_properties);
static void proxy_added_cb(GDBusProxy *new_proxy);
static void proxy_removed_cb(GDBusProxy *removed_proxy);

void bt_player_init()
{
    pthread_mutex_init(&data.proxy_mtx, NULL);
    data.proxy_watch = dbus_add_proxy_watch(proxy_filter, proxy_added_cb, proxy_removed_cb);
}

void bt_player_cleanup()
{
    dbus_remove_proxy_watch(data.proxy_watch);
    g_free(props.track_info.title);
    g_free(props.track_info.artist);
    g_free(props.track_info.album);
    g_free(props.track_info.genre);
}

bool bt_player_is_connected()
{
    return data.player_proxy;
}

bool bt_player_send_command(bt_player_cmd_e command)
{
    GError *error = NULL;
    bool ret = true;

    pthread_mutex_lock(&data.proxy_mtx);

    if (!data.player_proxy)
    {
        ret = false;
    }
    else if (g_dbus_proxy_call_sync(data.player_proxy,
                                    COMMAND_STRINGS[command],
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error) == NULL)
    {
        g_printerr("%s\n", error->message);
        ret = false;
    }

    pthread_mutex_unlock(&data.proxy_mtx);

    return ret;
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

    if (cb_data)
    {
        cb_data->cb(success, cb_data->user_data);
        g_free(cb_data);
    }
}

void bt_player_send_command_async(bt_player_cmd_e command,
                                  bt_player_cmd_cb callback,
                                  void *user_data)
{
    cmd_cb_data_t *cb_data = NULL;

    if (callback)
    {
        cb_data = g_new0(cmd_cb_data_t, 1);
        cb_data->cb = callback;
        cb_data->user_data = user_data;
    }

    pthread_mutex_lock(&data.proxy_mtx);

    g_dbus_proxy_call(data.player_proxy,
                      COMMAND_STRINGS[command],
                      NULL,
                      G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      send_command_async_done,
                      cb_data);

    pthread_mutex_unlock(&data.proxy_mtx);
}

bool bt_player_get_property(bt_player_prop_e property, void *retval)
{
    prop_entry_t *entry;
    GVariant *val;
    bool ret;

    entry = &prop_entries[property];

    pthread_mutex_lock(&data.proxy_mtx);
    if (!*entry->proxy)
    {
        g_printerr("Proxy for property '%s' is unavailable\n",
                   entry->prop_name);
        ret = false;
    }
    else if (!(val = g_dbus_proxy_get_cached_property(*entry->proxy, entry->prop_name)))
    {
        g_printerr("Property '%s' is unavailable\n", entry->prop_name);
        ret = false;
    }
    pthread_mutex_unlock(&data.proxy_mtx);

    if (!ret)
        return ret;

    pthread_mutex_lock(&entry->mtx);
    entry->update_val_fn(val);
    g_variant_unref(val);

    switch (property)
    {
    case BT_PLAYER_PROP_PLAYBACK_STATUS:
        *(bt_player_status_e *)retval = props.status;
        break;
    case BT_PLAYER_PROP_SHUFFLE_MODE:
        *(bt_player_shuffle_e *)retval = props.shuffle;
        break;
    case BT_PLAYER_PROP_REPEAT_MODE:
        *(bt_player_repeat_e *)retval = props.repeat;
        break;
    case BT_PLAYER_PROP_PLAYBACK_POSITION:
        *(uint32_t *)retval = props.position;
        break;
    case BT_PLAYER_PROP_TRACK:
        bt_player_track_info_t *track_ret = retval;
        track_ret->title = g_strdup(props.track_info.title);
        track_ret->artist = g_strdup(props.track_info.artist);
        track_ret->album = g_strdup(props.track_info.album);
        track_ret->genre = g_strdup(props.track_info.genre);
        track_ret->track_count = props.track_info.track_count;
        track_ret->track_number = props.track_info.track_number;
        track_ret->duration = props.track_info.duration;
        break;
    case BT_PLAYER_PROP_VOLUME:
        *(uint8_t *)retval = props.volume;
        break;
    default:
        ret = false;
    }

    pthread_mutex_unlock(&entry->mtx);

    return ret;
}

bool bt_player_set_property(bt_player_prop_e property, void *arg)
{
    const prop_entry_t *prop_entry;
    GVariant *value;
    GError *error;
    bool ret;

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
        return false;
    }

    ret = true;
    error = NULL;
    pthread_mutex_lock(&data.proxy_mtx);
    if (!*prop_entry->proxy)
    {
        g_printerr("Proxy for property '%s' is unavailable\n",
                   prop_entry->prop_name);
        ret = false;
    }
    else if (!dbus_proxy_get_property(*prop_entry->proxy,
                                      prop_entry->prop_name,
                                      &error) ||
             !dbus_proxy_set_property(*prop_entry->proxy,
                                      prop_entry->prop_name,
                                      value,
                                      &error))
    {
        g_printerr("%s\n", error->message);
        ret = false;
    }
    pthread_mutex_unlock(&data.proxy_mtx);

    return ret;
}

void bt_player_set_property_changed_cb(bt_player_prop_e property,
                                       bt_player_prop_cb callback,
                                       void *user_data)
{
    data.prop_cb_data[property].cb = callback;
    data.prop_cb_data[property].user_data = user_data;
}

static bool proxy_filter(GDBusProxy *proxy)
{
    const gchar *iface_name = g_dbus_proxy_get_interface_name(proxy);
    return g_str_equal(iface_name, BLUEZ_MEDIA_PLAYER_INTERFACE) ||
           g_str_equal(iface_name, BLUEZ_MEDIA_TRANSPORT_INTERFACE);
}

static void proxy_changed_cb(GDBusProxy *proxy,
                             GVariant *changed_properties,
                             const gchar *const *invalidated_properties)
{
    (void)proxy;
    (void)invalidated_properties;

    if (g_variant_n_children(changed_properties) == 0)
        return;

    GVariantIter *iter;
    const gchar *key;
    GVariant *value;

    g_variant_get(changed_properties, "a{sv}", &iter);

    while (g_variant_iter_loop(iter, "{&sv}", &key, &value))
    {
        for (size_t prop_id = 0; prop_id < BT_PLAYER_PROP_cnt; prop_id++)
        {
            if (g_str_equal(prop_entries[prop_id].prop_name, key))
            {
                prop_entry_t *entry = &prop_entries[prop_id];
                prop_cb_data_t *cb_data = &data.prop_cb_data[prop_id];

                pthread_mutex_lock(&entry->mtx);

                if (entry->update_val_fn(value) && cb_data->cb)
                    cb_data->cb(entry->prop_val, cb_data->user_data);

                pthread_mutex_unlock(&entry->mtx);

                break;
            }
        }
    }
    g_variant_iter_free(iter);
}

static void proxy_added_cb(GDBusProxy *new_proxy)
{
    GDBusProxy **proxy;
    gulong *handler_id;
    const gchar *iface_name;

    iface_name = g_dbus_proxy_get_interface_name(new_proxy);

    pthread_mutex_lock(&data.proxy_mtx);
    if (g_str_equal(iface_name, BLUEZ_MEDIA_PLAYER_INTERFACE))
    {
        g_print("player connected\n");
        proxy = &data.player_proxy;
        handler_id = &data.player_handler_id;
    }
    else if (g_str_equal(iface_name, BLUEZ_MEDIA_TRANSPORT_INTERFACE))
    {
        g_print("transport connected\n");
        proxy = &data.transport_proxy;
        handler_id = &data.transport_handler_id;
    }
    else
    {
        pthread_mutex_unlock(&data.proxy_mtx);
        return;
    }

    if (*proxy)
    {
        g_signal_handler_disconnect(*proxy,
                                    *handler_id);
        g_object_unref(*proxy);
    }

    *proxy = new_proxy;

    for (size_t i = 0; i < BT_PLAYER_PROP_cnt; i++)
    {
        prop_entry_t *entry = &prop_entries[i];
        if (*entry->proxy == *proxy)
        {
            GVariant *prop_val;

            if ((prop_val = g_dbus_proxy_get_cached_property(*entry->proxy, entry->prop_name)))
            {
                pthread_mutex_lock(&entry->mtx);
                entry->update_val_fn(prop_val);
                pthread_mutex_unlock(&entry->mtx);
                g_variant_unref(prop_val);
            }
        }
    }

    *handler_id = g_signal_connect(*proxy,
                                   "g-properties-changed",
                                   G_CALLBACK(proxy_changed_cb),
                                   NULL);
    pthread_mutex_unlock(&data.proxy_mtx);
}

static void proxy_removed_cb(GDBusProxy *removed_proxy)
{
    pthread_mutex_lock(&data.proxy_mtx);
    if (data.player_proxy == removed_proxy)
    {
        g_signal_handler_disconnect(data.player_proxy,
                                    data.player_handler_id);
        g_object_unref(data.player_proxy);
        data.player_proxy = NULL;
    }
    else if (data.transport_proxy == removed_proxy)
    {
        g_signal_handler_disconnect(data.transport_proxy,
                                    data.transport_handler_id);
        g_object_unref(data.transport_proxy);
        data.player_proxy = NULL;
    }
    pthread_mutex_unlock(&data.proxy_mtx);
}

// Update status property only if new val different from old val. Return true if changed
static bool prop_update_status(GVariant *val)
{
    const gchar *status_str = g_variant_get_string(val, NULL);
    bt_player_status_e new_status = props.status;

    if (g_str_equal(status_str, "stopped"))
        new_status = BT_PLAYER_STATUS_STOPPED;
    else if (g_str_equal(status_str, "playing"))
        new_status = BT_PLAYER_STATUS_PLAYING;
    else if (g_str_equal(status_str, "paused"))
        new_status = BT_PLAYER_STATUS_PAUSED;

    if (new_status == props.status)
        return false;

    props.status = new_status;
    return true;
}

// Update shuffle property only if new val different from old val. Return true if changed
static bool prop_update_shuffle(GVariant *val)
{
    const gchar *shuffle_str = g_variant_get_string(val, NULL);
    bt_player_shuffle_e new_shuffle = BT_PLAYER_SHUFFLE_OFF;

    if (g_str_equal(shuffle_str, "alltracks"))
        new_shuffle = BT_PLAYER_SHUFFLE_ALL_TRACKS;

    if (new_shuffle == props.shuffle)
        return false;

    props.shuffle = new_shuffle;
    return true;
}

// Update repeat property only if new val different from old val. Return true if changed
static bool prop_update_repeat(GVariant *val)
{
    const gchar *repeat_str = g_variant_get_string(val, NULL);
    bt_player_repeat_e new_repeat = BT_PLAYER_REPEAT_OFF;

    if (g_str_equal(repeat_str, "singletrack"))
        props.repeat = BT_PLAYER_REPEAT_SINGLE_TRACK;
    else if (g_str_equal(repeat_str, "alltracks"))
        props.repeat = BT_PLAYER_REPEAT_ALL_TRACKS;

    if (new_repeat == props.repeat)
        return false;

    props.repeat = new_repeat;
    return true;
}

// Update position property only if new val different from old val. Return true if changed
static bool prop_update_position(GVariant *val)
{
    uint32_t new_pos = g_variant_get_uint32(val);
    if (new_pos == props.position)
        return false;

    props.position = new_pos;
    return true;
}

static bool str_equal(const char *s1, const char *s2)
{
    if (s1 == NULL || s2 == NULL)
        return s1 == s2;

    return strcmp(s1, s2) == 0;
}

static bool track_equal(const bt_player_track_info_t *t1, const bt_player_track_info_t *t2)
{
    return str_equal(t1->title, t2->title) && str_equal(t1->artist, t2->artist) &&
           str_equal(t1->album, t2->album) && str_equal(t1->genre, t2->genre) &&
           t1->track_count == t2->track_count && t1->track_number == t2->track_number &&
           t1->duration == t2->duration;
}

// Update track property only if new val different from old val. Return true if changed
static bool prop_update_track(GVariant *val)
{
    GVariantDict dict;

    bt_player_track_info_t new_track = {0};

    g_variant_dict_init(&dict, val);
    g_variant_dict_lookup(&dict, "Title", "&s", &new_track.title);
    g_variant_dict_lookup(&dict, "Artist", "&s", &new_track.artist);
    g_variant_dict_lookup(&dict, "Album", "&s", &new_track.album);
    g_variant_dict_lookup(&dict, "Genre", "&s", &new_track.genre);
    g_variant_dict_lookup(&dict, "NumberOfTracks", "u", &new_track.track_count);
    g_variant_dict_lookup(&dict, "TrackNumber", "u", &new_track.track_number);
    g_variant_dict_lookup(&dict, "Duration", "u", &new_track.duration);

    if (track_equal(&new_track, &props.track_info))
        return false;

    g_free(props.track_info.title);
    g_free(props.track_info.artist);
    g_free(props.track_info.album);
    g_free(props.track_info.genre);

    props.track_info = (bt_player_track_info_t){
        .title = new_track.title ? g_strdup(new_track.title) : NULL,
        .artist = new_track.artist ? g_strdup(new_track.artist) : NULL,
        .album = new_track.album ? g_strdup(new_track.album) : NULL,
        .genre = new_track.genre ? g_strdup(new_track.genre) : NULL,
        .track_count = new_track.track_count,
        .track_number = new_track.track_number,
        .duration = new_track.duration,
    };

    return true;
}

static bool prop_update_volume(GVariant *val)
{
    uint16_t raw_vol = g_variant_get_uint16(val);
    raw_vol = raw_vol > 127 ? 127 : raw_vol;

    if (raw_vol == props.volume)
        return false;

    props.volume = raw_vol;
    return true;
}

void bt_player_set_pause_sync(GDBusProxy *player_proxy)
{
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
    }
    else
    {
        g_printerr("\nSuccess pause\n");
    }
}

void bt_player_set_play_sync(GDBusProxy *player_proxy)
{
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
    }
    else
    {
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

void print_track_data(GDBusProxy *player_proxy)
{

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
    if (title_start != NULL)
    {
        title_start += 11; // Skip "'Title': <'"
        gchar *title_end = strstr(title_start, "'>");
        if (title_end != NULL)
        {
            gint title_length = title_end - title_start;
            title = g_strndup(title_start, title_length);

            if (title_length < MAX_STRING_LENGTH)
            {
                strncpy(current_title, title_start, title_length);
                current_title[title_length] = '\0';
            }
            else
            {
                // If string is too long, just copy the maximum allowed length
                strncpy(current_title, title_start, MAX_STRING_LENGTH - 1);
                current_title[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    // Parsing 'TrackNumber'
    gchar *track_num_start = strstr(str, "'TrackNumber': <");
    if (track_num_start != NULL)
    {
        track_num_start += 23;
        gchar *track_num_end = strstr(track_num_start, ">");
        if (track_num_end != NULL)
        {
            gint track_num_length = track_num_end - track_num_start;
            track_number = g_strndup(track_num_start, track_num_length);
        }
    }
    int tarck_num_test = (int)g_ascii_strtoull(track_number, NULL, 10);

    // Parsing 'NumberOfTracks'
    gchar *num_tracks_start = strstr(str, "'NumberOfTracks': <");
    if (num_tracks_start != NULL)
    {
        num_tracks_start += 26;
        gchar *num_tracks_end = strstr(num_tracks_start, ">");
        if (num_tracks_end != NULL)
        {
            gint num_tracks_length = num_tracks_end - num_tracks_start;
            num_tracks = g_strndup(num_tracks_start, num_tracks_length);
        }
    }
    int num_tracks_test = (int)g_ascii_strtoull(num_tracks, NULL, 10);

    // Parsing 'Duration'
    gchar *duration_start = strstr(str, "'Duration': <uint32 ");
    if (duration_start != NULL)
    {
        duration_start += 19;
        gchar *duration_end = strstr(duration_start, ">");
        if (duration_end != NULL)
        {
            gint duration_length = duration_end - duration_start;
            duration = g_strndup(duration_start, duration_length);
        }
    }
    int dur_test = (int)g_ascii_strtoull(duration, NULL, 10);

    // Parsing 'Album'
    gchar *album_start = strstr(str, "'Album': <'");
    if (album_start != NULL)
    {
        album_start += 11;
        gchar *album_end = strstr(album_start, "'>");
        if (album_end != NULL)
        {
            gint album_length = album_end - album_start;
            album = g_strndup(album_start, album_length);

            if (album_length < MAX_STRING_LENGTH)
            {
                strncpy(current_album, album_start, album_length);
                current_album[album_length] = '\0';
            }
            else
            {
                strncpy(current_album, album_start, MAX_STRING_LENGTH - 1);
                current_album[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    // Parsing 'Artist'
    gchar *artist_start = strstr(str, "'Artist': <'");
    if (artist_start != NULL)
    {
        artist_start += 12;
        gchar *artist_end = strstr(artist_start, "'>");
        if (artist_end != NULL)
        {
            gint artist_length = artist_end - artist_start;
            artist = g_strndup(artist_start, artist_length);

            if (artist_length < MAX_STRING_LENGTH)
            {
                strncpy(current_artist, artist_start, artist_length);
                current_artist[artist_length] = '\0';
            }
            else
            {
                strncpy(current_artist, artist_start, MAX_STRING_LENGTH - 1);
                current_artist[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    property = g_dbus_proxy_get_cached_property(player_proxy, "Status");
    gchar *str_other = g_variant_print(property, FALSE);

    gchar *status_start = strstr(str_other, "'");
    if (status_start != NULL)
    {
        status_start += 1;
        gchar *status_end = strstr(status_start, "'");
        if (status_end != NULL)
        {
            gint status_length = status_end - status_start;
            status = g_strndup(status_start, status_length);

            if (status_length < MAX_STRING_LENGTH)
            {
                strncpy(current_track_status, status_start, status_length);
                current_track_status[status_length] = '\0';
            }
            else
            {
                strncpy(current_track_status, status_start, MAX_STRING_LENGTH - 1);
                current_track_status[MAX_STRING_LENGTH - 1] = '\0';
            }
        }
    }

    property = g_dbus_proxy_get_cached_property(player_proxy, "Position");
    str_other = g_variant_print(property, FALSE);
    int curr_pos_test = (int)g_ascii_strtoull(str_other, NULL, 10);

    if (current_track_status[0] != '\0')
    {
        printf("Status: '%s'\n", current_track_status);
    }
    else
    {
        printf("Artist: Unknown\n");
    }
    if (current_title[0] != '\0')
    {
        printf("Artist: '%s'\n", current_title);
    }
    else
    {
        printf("Artist: Unknown\n");
    }

    printf("Track Number: %d\n", tarck_num_test);
    printf("Number of Tracks: %d\n", num_tracks_test);
    printf("Duration: %d\n", dur_test ? dur_test : -1);
    printf("Current Position: %d\n", curr_pos_test ? curr_pos_test : -1);

    if (current_album[0] != '\0')
    {
        printf("Album: '%s'\n", current_album);
    }
    else
    {
        printf("Album: Unknown\n");
    }
    if (current_artist[0] != '\0')
    {
        printf("Artist: '%s'\n", current_artist);
    }
    else
    {
        printf("Artist: Unknown\n");
    }

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

bool is_paused_status()
{
    if (strcmp(current_track_status, "paused") == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}