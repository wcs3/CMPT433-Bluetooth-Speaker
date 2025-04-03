#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <glib.h>
#include <gio/gio.h>

#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"
#define BLUEZ_NAME "bluez.org"
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

            bool found_watcher = false;
            for (GList *lll = proxy_watchers; lll; lll = lll->next)
            {
                proxy_watch_t *watch = lll->data;
                if (watch->filter && watch->filter(proxy) && watch->added_cb)
                {
                    watch->added_cb(proxy);
                    found_watcher = true;
                }
            }
            if (!found_watcher)
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
