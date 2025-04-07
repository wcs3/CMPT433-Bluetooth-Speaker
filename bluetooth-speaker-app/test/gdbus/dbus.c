#include "dbus.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include <glib.h>

#define DBUS_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define BLUEZ_NAME "org.bluez"
#define BLUEZ_OBJECT_MANAGER_PATH "/"

struct proxy_watch_t
{
    dbus_proxy_watch_filter_cb filter;
    dbus_proxy_watch_added_cb added_cb;
    dbus_proxy_watch_removed_cb removed_cb;
};

static GList *watchers_list = NULL;
pthread_mutex_t watchers_list_mtx = PTHREAD_MUTEX_INITIALIZER;

static GMainLoop *main_loop;
static GDBusObjectManager *manager;

static pthread_t dbus_thread_id;

static void *dbus_routine(void *arg);

static void on_notify_name_owner(GObject *object, GParamSpec *pspec, gpointer user_data);
static void on_object_added(GDBusObjectManager *manager,
                            GDBusObject *object,
                            gpointer user_data);
static void on_object_removed(GDBusObjectManager *manager,
                              GDBusObject *object,
                              gpointer user_data);

static void preload_watcher(dbus_proxy_watch_t *watch);

void dbus_init()
{
    GError *error = NULL;

    manager = g_dbus_object_manager_client_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                            BLUEZ_NAME,
                                                            BLUEZ_OBJECT_MANAGER_PATH,
                                                            NULL, NULL, NULL, NULL,
                                                            &error);

    if (manager == NULL)
    {
        g_printerr("Error getting object manager client: %s", error->message);
        exit(EXIT_FAILURE);
    }

    gchar *name_owner = g_dbus_object_manager_client_get_name_owner(G_DBUS_OBJECT_MANAGER_CLIENT(manager));
    g_print("name-owner: %s\n", name_owner);
    g_free(name_owner);

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

    if (pthread_create(&dbus_thread_id, NULL, dbus_routine, NULL) < 0)
    {
        perror("dbus thread create");
        exit(EXIT_FAILURE);
    }
}

void dbus_cleanup()
{
    g_main_loop_quit(main_loop);

    if (pthread_join(dbus_thread_id, NULL) < 0)
    {
        perror("dbus thread join");
        exit(EXIT_FAILURE);
    }

    g_object_unref(manager);

    pthread_mutex_lock(&watchers_list_mtx);
    g_list_free_full(watchers_list, g_free);
    pthread_mutex_unlock(&watchers_list_mtx);
}

static void *dbus_routine(void *arg)
{
    (void)arg;

    GMainContext *main_context = g_main_context_get_thread_default();
    main_loop = g_main_loop_new(main_context, FALSE);

    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);

    return NULL;
}

dbus_proxy_watch_t *dbus_add_proxy_watch(dbus_proxy_watch_filter_cb filter_cb,
                                         dbus_proxy_watch_added_cb added_cb,
                                         dbus_proxy_watch_removed_cb removed_cb)
{
    g_assert(filter_cb);
    g_assert(added_cb || removed_cb);

    dbus_proxy_watch_t *watch = g_new(dbus_proxy_watch_t, 1);
    watch->filter = filter_cb;
    watch->added_cb = added_cb;
    watch->removed_cb = removed_cb;

    if (watch->added_cb)
        preload_watcher(watch);

    pthread_mutex_lock(&watchers_list_mtx);
    watchers_list = g_list_append(watchers_list, watch);
    pthread_mutex_unlock(&watchers_list_mtx);

    return watch;
}

void dbus_remove_proxy_watch(dbus_proxy_watch_t *watch)
{
    pthread_mutex_lock(&watchers_list_mtx);
    if (g_list_find(watchers_list, watch))
    {
        watchers_list = g_list_remove(watchers_list, watch);
        g_free(watch);
    }
    pthread_mutex_unlock(&watchers_list_mtx);
}

GVariant *dbus_proxy_get_property(GDBusProxy *proxy,
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

    g_variant_unref(param);
}

bool dbus_proxy_set_property(GDBusProxy *proxy,
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

    g_variant_unref(param);
}

static void preload_watcher(dbus_proxy_watch_t *watch)
{
    GList *objects;
    GList *l;

    objects = g_dbus_object_manager_get_objects(manager);
    for (l = objects; l != NULL; l = l->next)
    {
        GDBusObject *object = G_DBUS_OBJECT(l->data);
        GList *interfaces;
        GList *ll;

        interfaces = g_dbus_object_get_interfaces(G_DBUS_OBJECT(object));
        for (ll = interfaces; ll != NULL; ll = ll->next)
        {
            GDBusProxy *proxy = G_DBUS_PROXY(ll->data);

            if (watch->filter(proxy))
            {
                g_object_ref(proxy);
                watch->added_cb(proxy);
            }
            g_object_unref(proxy);
        }
        g_list_free(interfaces);
    }
    g_list_free_full(objects, g_object_unref);
}

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

static void on_object_added(GDBusObjectManager *manager,
                            GDBusObject *object,
                            gpointer user_data)
{
    (void)manager;
    (void)user_data;

    GList *proxy_list = g_dbus_object_get_interfaces(object);

    for (GList *l = proxy_list; l; l = l->next)
    {
        GDBusProxy *proxy = l->data;

        pthread_mutex_lock(&watchers_list_mtx);
        for (GList *ll = watchers_list; ll; ll = ll->next)
        {
            dbus_proxy_watch_t *watch = ll->data;

            if (watch->filter(proxy) && watch->added_cb)
            {
                g_object_ref(proxy);
                watch->added_cb(proxy);
            }
        }
        pthread_mutex_unlock(&watchers_list_mtx);
    }
    g_list_free_full(proxy_list, g_object_unref);
}

static void on_object_removed(GDBusObjectManager *manager,
                              GDBusObject *object,
                              gpointer user_data)
{
    (void)manager;
    (void)user_data;

    GList *proxy_list = g_dbus_object_get_interfaces(object);

    for (GList *iter = proxy_list; iter; iter = iter->next)
    {
        GDBusProxy *proxy = iter->data;

        pthread_mutex_lock(&watchers_list_mtx);
        for (GList *iter2 = watchers_list; iter2; iter2 = iter2->next)
        {
            dbus_proxy_watch_t *watch = iter2->data;
            if (watch->filter(proxy) && watch->removed_cb)
                watch->removed_cb(proxy);
        }
        pthread_mutex_unlock(&watchers_list_mtx);
    }

    g_list_free_full(proxy_list, g_object_unref);
}