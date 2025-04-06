#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <glib.h>
#include <gio/gio.h>

#include "player.h"


#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"
#define BLUEZ_NAME "org.bluez"
#define BLUEZ_OBJECT_MANAGER_PATH "/"

typedef struct
{
    bool (*filter)(const GDBusProxy *);
    void (*added_cb)(GDBusProxy *);
    void (*removed_cb)(GDBusProxy *);
    void (*changed_cb)(GDBusProxy *, GVariant *);
} proxy_watch_t;

static GList *proxy_watchers = NULL;

static GMainLoop *main_loop;
static GDBusConnection *dbus_conn;
static GDBusProxy *default_player;
static GSList *players = NULL;


///////////////////////// PARSING ///////////////////

#include <string.h>

#define MAX_STRING_LENGTH 256

char current_artist[MAX_STRING_LENGTH];
char current_title[MAX_STRING_LENGTH];
char current_album[MAX_STRING_LENGTH];
char current_track_status[MAX_STRING_LENGTH];

void print_track_data(const gchar *str) {
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

    GVariant *property = g_dbus_proxy_get_cached_property(default_player, "Status");
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

    property = g_dbus_proxy_get_cached_property(default_player, "Position");
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
    g_free(title);
    g_free(track_number);
    g_free(num_tracks);
    g_free(duration);
    g_free(album);
    g_free(artist);
    g_free(status);
    g_free(str_other);
}

/////////////////////////

static void on_notify_name_owner(GObject *object, GParamSpec *pspec, gpointer user_data)
{
    (void)pspec;
    (void)user_data;

    GDBusObjectManagerClient *manager = G_DBUS_OBJECT_MANAGER_CLIENT(object);
    gchar *name_owner;

    name_owner = g_dbus_object_manager_client_get_name_owner(manager);
    if (*name_owner == '\0')
        g_print("disconnected\n");
    else
        g_print("connected. name-owner: %s\n", name_owner);
    g_free(name_owner);
}

static char *proxy_description(GDBusProxy *proxy, const char *title,
                               const char *description)
{
    const char *path;

    path = g_dbus_proxy_get_object_path(proxy);

    return g_strdup_printf("%s%s%s%s %s ",
                           description ? "[" : "",
                           description ? description : "",
                           description ? "] " : "",
                           title, path);
}

static void print_player(GDBusProxy *proxy, const char *description)
{
    char *str;

    str = proxy_description(proxy, "Player", description);

    g_print("%s%s\n", str, default_player == proxy ? "[default]" : "");

    g_free(str);
}

void cmd_list()
{
    GSList *l;

    for (l = players; l; l = g_slist_next(l))
    {
        GDBusProxy *proxy = l->data;
        print_player(proxy, NULL);
    }
}   

static void print_property(GDBusProxy *proxy, const char *name)
{
    GVariant *property;

    if ((property = g_dbus_proxy_get_cached_property(proxy, name)) == NULL)
        return;

    gchar *str = g_variant_print(property, FALSE);
    g_print("%s\n", str);
    g_free(str);
}

void cmd_show()
{
    GDBusProxy *proxy;

    proxy = default_player;

    g_print("Player %s\n", g_dbus_proxy_get_object_path(proxy));

    print_property(proxy, "Name");
    print_property(proxy, "Repeat");
    print_property(proxy, "Equalizer");
    print_property(proxy, "Shuffle");
    print_property(proxy, "Scan");
    print_property(proxy, "Status");
    print_property(proxy, "Position");
    print_property(proxy, "Track");
}

static void player_added(GDBusProxy *proxy)
{
    players = g_slist_append(players, proxy);

    if (default_player == NULL)
        default_player = players->data;

    print_player(proxy, "NEW");
}

static void on_object_added(GDBusObjectManager *manager,
                            GDBusObject *object,
                            gpointer user_data)
{
    (void)manager;
    (void)user_data;

    g_print("Object '%s' added", g_dbus_object_get_object_path(object));

    GList *proxy_list = g_dbus_object_get_interfaces(object);

    if (proxy_list)
        g_print(" with interfaces:");

    g_print("\n");

    for (GList *iter = proxy_list; iter; iter = iter->next)
    {
        GDBusProxy *proxy = iter->data;
        const gchar *iface_name = g_dbus_proxy_get_interface_name(proxy);

        g_print("\t'%s'\n", iface_name);

        if (g_str_equal(iface_name, BLUEZ_MEDIA_PLAYER_INTERFACE))
            player_added(proxy);
    }
}

static void on_object_removed(GDBusObjectManager *manager,
                              GDBusObject *object,
                              gpointer user_data)
{
    (void)manager;
    (void)user_data;

    g_print("Object '%s' removed", g_dbus_object_get_object_path(object));

    GList *proxy_list = g_dbus_object_get_interfaces(object);

    for (GList *iter = proxy_list; iter; iter = iter->next)
    {
        GDBusProxy *proxy = iter->data;

        for (GList *iter2 = proxy_watchers; iter2; iter2 = iter2->next)
        {
            proxy_watch_t *watch = iter2->data;
            if (watch->filter && watch->filter(proxy) && watch->removed_cb)
                watch->removed_cb(proxy);
        }
    }

    g_list_free(proxy_list);
}

static void on_property_changed(GDBusObjectManagerClient *manager,
                                GDBusObjectProxy *object_proxy,
                                GDBusProxy *interface_proxy,
                                GVariant *changed_properties,
                                const gchar *const *invalidated_properties,
                                gpointer user_data)
{
    (void)manager;
    (void)object_proxy;
    (void)invalidated_properties;
    (void)user_data;

    for (GList *iter = proxy_watchers; iter; iter = iter->next)
    {
        proxy_watch_t *watch = iter->data;
        if (watch->filter && watch->filter(interface_proxy) && watch->changed_cb)
            watch->changed_cb(interface_proxy, changed_properties);
    }
}

static void load_existing_proxies(GDBusObjectManager *manager)
{
    GList *objects;
    GList *l;

    g_print("Object manager at %s\n", g_dbus_object_manager_get_object_path(manager));
    objects = g_dbus_object_manager_get_objects(manager);
    for (l = objects; l != NULL; l = l->next)
    {
        GDBusObject *object = G_DBUS_OBJECT(l->data);
        GList *interfaces;
        GList *ll;
        g_print(" - Object at %s\n", g_dbus_object_get_object_path(G_DBUS_OBJECT(object)));

        interfaces = g_dbus_object_get_interfaces(G_DBUS_OBJECT(object));
        for (ll = interfaces; ll != NULL; ll = ll->next)
        {
            GDBusProxy *proxy = G_DBUS_PROXY(ll->data);

            const gchar *iface_name = g_dbus_proxy_get_interface_name(proxy);

            // g_print("\t'%s'\n", iface_name);

            if (g_str_equal(iface_name, BLUEZ_MEDIA_PLAYER_INTERFACE))
                player_added(proxy);

            for (GList *lll = proxy_watchers; lll; lll = lll->next)
            {
                proxy_watch_t *watch = lll->data;
                if (watch->filter && watch->filter(proxy) && watch->added_cb)
                {
                    g_object_ref(proxy);
                    watch->added_cb(proxy);
                }
            }
            g_object_unref(proxy);
        }
        g_list_free(interfaces);
    }
    g_list_free_full(objects, g_object_unref);
}

gboolean callback(gpointer data)
{
    g_slist_free_full(players, g_object_unref);

    g_main_loop_quit((GMainLoop *)data);
    return FALSE;
}

static void cleanup_handler(int signo)
{
    if (signo == SIGINT)
    {
        callback(main_loop);
    }
}

int main()
{
    GDBusObjectManager *manager;
    GError *error;

    manager = NULL;
    main_loop = NULL;

    main_loop = g_main_loop_new(NULL, FALSE);
    dbus_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    signal(SIGINT, cleanup_handler);

    error = NULL;
    manager = g_dbus_object_manager_client_new_sync(dbus_conn,
                                                    G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                    BLUEZ_NAME,
                                                    BLUEZ_OBJECT_MANAGER_PATH,
                                                    NULL, NULL, NULL, NULL,
                                                    &error);

    if (manager == NULL)
    {
        g_printerr("Error getting object manager client: %s", error->message);
        g_error_free(error);
        goto out;
    }

    gchar *name_owner = g_dbus_object_manager_client_get_name_owner(G_DBUS_OBJECT_MANAGER_CLIENT(manager));
    g_print("name-owner: %s\n", name_owner);
    g_free(name_owner);

    load_existing_proxies(manager);

    // cmd_show();


    ////////////////////////////// testing //////////////////

    GVariant *property = g_dbus_proxy_get_cached_property(default_player, "Track");
    gchar *str = g_variant_print(property, FALSE);

    print_track_data(str);

    

    if(strcmp(current_track_status, "playing") == 0){
        printf("\nMusic is on!\n");
        // bt_player_send_command(BT_PLAYER_CMD_PAUSE);
        GError *error = NULL;

        if (g_dbus_proxy_call_sync(default_player,
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
            printf("\nSuccess pause\n");
        }
    } else if(strcmp(current_track_status, "paused") == 0){
        printf("\nMusic is off!\n");
        // bt_player_send_command(BT_PLAYER_CMD_PLAY);
        GError *error = NULL;

        if (g_dbus_proxy_call_sync(default_player,
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
        printf("\nSuccess play\n");
        }
    }

    error = NULL;

    if (g_dbus_proxy_call_sync(default_player,
                                "Next",
                                NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1,
                                NULL,
                                &error) == NULL)
        {
        g_printerr("%s\n", error->message);
        // return false;
    } else {
        printf("\nSuccess Next\n");
    }

    g_free(str);
    printf("\nthe end\n");

    ////////////////////////////// testing END //////////////////


    g_signal_connect(manager,
                     "notify::name-owner",
                     G_CALLBACK(on_notify_name_owner),
                     NULL);
    g_signal_connect(manager,
                     "object-added",
                     G_CALLBACK(on_object_added),
                     NULL);
    g_signal_connect(manager,
                     "object-removed",
                     G_CALLBACK(on_object_removed),
                     NULL);
    g_signal_connect(manager,
                     "interface-proxy-properties-changed",
                     G_CALLBACK(on_property_changed),
                     NULL);

    g_main_loop_run(main_loop);

out:
    if (manager != NULL)
        g_object_unref(manager);

    if (main_loop != NULL)
        g_main_loop_unref(main_loop);

    g_dbus_connection_close_sync(dbus_conn, NULL, NULL);
    g_object_unref(dbus_conn);

    return 0;
}
