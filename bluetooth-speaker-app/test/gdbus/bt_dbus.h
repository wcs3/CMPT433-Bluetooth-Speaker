#ifndef __DBUS_H__
#define __DBUS_H__

#include <stdbool.h>
#include <gio/gio.h>

void dbus_init();
void dbus_cleanup();

typedef struct proxy_watch_t dbus_proxy_watch_t;

typedef bool (*dbus_proxy_watch_filter_cb)(GDBusProxy *proxy);
typedef void (*dbus_proxy_watch_added_cb)(GDBusProxy *added_proxy);
typedef void (*dbus_proxy_watch_removed_cb)(GDBusProxy *removed_proxy);

dbus_proxy_watch_t *dbus_add_proxy_watch(dbus_proxy_watch_filter_cb filter_cb,
                                         dbus_proxy_watch_added_cb added_cb,
                                         dbus_proxy_watch_removed_cb removed_cb);

void dbus_remove_proxy_watch(dbus_proxy_watch_t *watch);

GVariant *dbus_proxy_get_property(GDBusProxy *proxy,
                                  const gchar *property_name,
                                  GError **error);
bool dbus_proxy_set_property(GDBusProxy *proxy,
                             const gchar *property_name,
                             GVariant *value,
                             GError **error);

#endif // __DBUS_H__