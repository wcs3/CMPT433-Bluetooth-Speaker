# BlueZ and glib Dbus Bindings

## Objects and Interfaces

service name: `org.bluez`

| Object Path                                     | Description       | Relevant Interfaces                                                                                                                                                                                                                  |
|-------------------------------------------------|-------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `/`                                             | BlueZ bus root    | [`org.freedesktop.DBus.ObjectManager`](https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager)                                                                                                   |
| `/org/bluez`                                    | BlueZ managers    | [`org.bluez.AgentManager1`](https://man.archlinux.org/man/org.bluez.AgentManager.5.en),                                                                                                                                              |
| `/org/bluez/hciX`                               | Bluetooth Adapter | [`org.bluez.Adapter1`](https://man.archlinux.org/man/extra/bluez-utils/org.bluez.Adapter.5.en)                                                                                                                                       |
| `/org/bluez/hciX/dev_XX_XX_XX_XX_XX_XX`         | Remote Device     | [`org.bluez.Device1`](https://man.archlinux.org/man/extra/bluez-utils/org.bluez.Device.5.en)                                                                                                                                         |
| `/org/bluez/hciX/dev_XX_XX_XX_XX_XX_XX/playerX` | Media Controller  | [`org.bluez.MediaPlayer1`](https://man.archlinux.org/man/extra/bluez-utils/org.bluez.MediaPlayer.5.en), [`org.freedesktop.DBus.Properties`](https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-properties) |
| `/org/bluez/hciX/dev_XX_XX_XX_XX_XX_XX/fdX`     | Media Transport   | [`org.bluez.MediaTransport1`](https://man.archlinux.org/man/extra/bluez-utils/org.bluez.MediaTransport.5.en)                                                                                                                         |

### Parsing an Object Path

Consider the object path `/org/bluez/hci0/dev_0C_02_BD_78_41_0B/player0`. This represents the `player0` object on the bluetooth device `dev_0C_02_BD_78_41_0B` on the adapter `hci0`.

### Getting Proxies

A [`GDBusProxy`](https://docs.gtk.org/gio/class.DBusProxy.html) is essentially a local copy of an interface of a remote DBus object that makes managing that object more convenient from a client application. To get proxies for BlueZ objects, we use a [`GDBusObjectManagerClient`](https://docs.gtk.org/gio/class.DBusObjectManagerClient.html), created from BlueZ's object manager which is at the path `/`. Calling [`g_dbus_object_manager_get_objects`](https://docs.gtk.org/gio/vfunc.DBusObjectManager.get_objects.html) on this manager will give us a list of `GDBusObjectProxy`s. Finally, calling [`g_dbus_object_get_interfaces`](https://docs.gtk.org/gio/vfunc.DBusObject.get_interfaces.html) on a `GDBusObjectProxy` gives a list of `GDBusProxy`s.

Now, given the list of `GDBusProxy`s, we can filter for a specific interface by checking if the `GDBusProxy` has the desired interface name. For example, if we want proxies for MediaPlayer objects, we might check:

```C
if (strequal(g_dbus_proxy_get_interface_name(proxy), "org.bluez.MediaPlayer1") == 0) {
    // save proxy
}
```

### Using Proxies

Once we have our hands on a `GDBusProxy`, it is fairly straightforward to call methods, get properties, and set properties on the proxy.

To call a method on a proxy asynchronously, use `g_dbus_proxy_call` with a callback that calls `g_dbus_proxy_call_finish`. To call a method synchronously, use `g_dbus_proxy_call_sync`.

To get a property from a proxy, use the `Get` method of the `org.freedesktop.DBus.Properties` interface on the proxies object. Likewise use the `Set` method of the `org.freedesktop.DBus.Properties` interface to set a property (Normally you would need a proxy to the `org.freedesktop.DBus.Properties` interface in order to call its methods, however, it is also possible to call a method of an object's interface using a different interface of the object: https://stackoverflow.com/questions/58494650/is-there-a-g-dbus-function-to-update-the-non-cached-property-value).

To get notified when a proxy's property changes, we can attach a callback to the proxy's [`g-properties-changed`](https://docs.gtk.org/gio/signal.DBusProxy.g-properties-changed.html) signal, or to avoid having to attach a callback on every proxy, we can use the [`interface-proxy-properties-changed`](https://docs.gtk.org/gio/signal.DBusObjectManagerClient.interface-proxy-properties-changed.html) signal of the `GDBusObjectManagerClient`, and get notified whenever a property changes on any object of the BlueZ.

## Useful Resources

- [DBus Tutorial](https://dbus.freedesktop.org/doc/dbus-tutorial.html): Good overview of DBus constructs
- [GVariant](https://docs.gtk.org/glib/struct.Variant.html): Used in GDBus to serialize parameters and return values of DBus method calls.

## Useful Tools

- `bluetoothctl`: A terminal client for BlueZ that demonstrates almost all of the capabilities available through BlueZ's DBus interface. Worth exploring how to setup agents, connect to devices, and use the player menu through this tool.
- `busctl`: A DBus utility that abstracts away the nitty gritty of interacting with DBus. I found this useful for exploring the BlueZ object tree and testing method calls/getting and setting properties. Use tab completion judiciously with this tool.
