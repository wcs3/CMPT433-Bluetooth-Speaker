// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2020  Intel Corporation. All rights reserved.
 *  Copyright 2023-2025 NXP
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
// #include <dbus/dbus.h>

// #include "lib/gdbus.h"

#include "lib/util.h"
#include "lib/bluetooth.h"
#include "lib/uuid.h"
#include "lib/iso.h"
#include "lib/shell.h"

#include "lib/print.h"

#include "player.h"

/* String display constants */
#define COLORED_NEW COLOR_GREEN "NEW" COLOR_OFF
#define COLORED_CHG COLOR_YELLOW "CHG" COLOR_OFF
#define COLORED_DEL COLOR_RED "DEL" COLOR_OFF

#define BLUEZ_MEDIA_INTERFACE "org.bluez.Media1"
#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"

static GDBusConnection *dbus_conn;
static GDBusProxy *default_player;
static GList *medias = NULL;
static GList *players = NULL;

static bool check_default_player(void)
{
    if (!default_player)
    {
        bt_shell_printf("No default player available\n");
        return FALSE;
    }

    return TRUE;
}

static char *generic_generator(const char *text, int state, GList *source)
{
    static int index = 0;

    if (!source)
        return NULL;

    if (!state)
        index = 0;

    return g_dbus_proxy_path_lookup(source, &index, text);
}

static char *player_generator(const char *text, int state)
{
    return generic_generator(text, state, players);
}

static void play_reply(GDBusMessage *message, void *user_data)
{
    GDBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to play: %s\n", error);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Play successful\n");

    return bt_shell_noninteractive_quit(EXIT_FAILURE);
}

static void cmd_play(int argc, char *argv[])
{
    GDBusProxy *proxy;

    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    proxy = default_player;

    if (g_dbus_proxy_method_call(proxy, "Play", NULL, play_reply,
                                 NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to play\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to play %s\n", argv[1] ?: "");
}

static void pause_reply(GDBusMessage *message, void *user_data)
{
    GDBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to pause: %s\n", error);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Pause successful\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_pause(int argc, char *argv[])
{
    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (g_dbus_proxy_method_call(default_player, "Pause", NULL,
                                 pause_reply, NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to play\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to pause\n");
}

static void stop_reply(GDBusMessage *message, void *user_data)
{
    GDBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to stop: %s\n", error);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Stop successful\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_stop(int argc, char *argv[])
{
    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (g_dbus_proxy_method_call(default_player, "Stop", NULL, stop_reply,
                                 NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to stop\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to stop\n");
}

static void next_reply(GDBusMessage *message, void *user_data)
{
    GDBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to jump to next: %s\n", error);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Next successful\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_next(int argc, char *argv[])
{
    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (g_dbus_proxy_method_call(default_player, "Next", NULL, next_reply,
                                 NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to jump to next\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to jump to next\n");
}

static void previous_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to jump to previous: %s\n", error.name);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Previous successful\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_previous(int argc, char *argv[])
{
    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (g_dbus_proxy_method_call(default_player, "Previous", NULL,
                                 previous_reply, NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to jump to previous\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to jump to previous\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void fast_forward_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to fast forward: %s\n", error.name);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("FastForward successful\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_fast_forward(int argc, char *argv[])
{
    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (g_dbus_proxy_method_call(default_player, "FastForward", NULL,
                                 fast_forward_reply, NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to jump to previous\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Fast forward playback\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void rewind_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE)
    {
        bt_shell_printf("Failed to rewind: %s\n", error.name);
        dbus_error_free(&error);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Rewind successful\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_rewind(int argc, char *argv[])
{
    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (g_dbus_proxy_method_call(default_player, "Rewind", NULL,
                                 rewind_reply, NULL, NULL) == FALSE)
    {
        bt_shell_printf("Failed to rewind\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Rewind playback\n");
}

static void generic_callback(const DBusError *error, void *user_data)
{
    char *str = user_data;

    if (dbus_error_is_set(error))
    {
        bt_shell_printf("Failed to set %s: %s\n", str, error->name);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }
    else
    {
        bt_shell_printf("Changing %s succeeded\n", str);
        return bt_shell_noninteractive_quit(EXIT_SUCCESS);
    }
}

static void cmd_equalizer(int argc, char *argv[])
{
    char *value;
    DBusMessageIter iter;

    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (!g_dbus_proxy_get_property(default_player, "Equalizer", &iter))
    {
        bt_shell_printf("Operation not supported\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    value = g_strdup(argv[1]);

    if (g_dbus_proxy_set_property_basic(default_player, "Equalizer",
                                        DBUS_TYPE_STRING, &value,
                                        generic_callback, value,
                                        g_free) == FALSE)
    {
        bt_shell_printf("Failed to setting equalizer\n");
        g_free(value);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to set equalizer\n");

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_repeat(int argc, char *argv[])
{
    char *value;
    DBusMessageIter iter;

    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (!g_dbus_proxy_get_property(default_player, "Repeat", &iter))
    {
        bt_shell_printf("Operation not supported\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    value = g_strdup(argv[1]);

    if (g_dbus_proxy_set_property_basic(default_player, "Repeat",
                                        DBUS_TYPE_STRING, &value,
                                        generic_callback, value,
                                        g_free) == FALSE)
    {
        bt_shell_printf("Failed to set repeat\n");
        g_free(value);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to set repeat\n");
}

static void cmd_shuffle(int argc, char *argv[])
{
    char *value;
    DBusMessageIter iter;

    if (!check_default_player())
        return bt_shell_noninteractive_quit(EXIT_FAILURE);

    if (!g_dbus_proxy_get_property(default_player, "Shuffle", &iter))
    {
        bt_shell_printf("Operation not supported\n");
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    value = g_strdup(argv[1]);

    if (g_dbus_proxy_set_property_basic(default_player, "Shuffle",
                                        DBUS_TYPE_STRING, &value,
                                        generic_callback, value,
                                        g_free) == FALSE)
    {
        bt_shell_printf("Failed to set shuffle\n");
        g_free(value);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    bt_shell_printf("Attempting to set shuffle\n");
}

static char *proxy_description(GDBusProxy *proxy, const char *title,
                               const char *description)
{
    const char *path;

    path = g_dbus_proxy_get_path(proxy);

    return g_strdup_printf("%s%s%s%s %s ",
                           description ? "[" : "",
                           description ?: "",
                           description ? "] " : "",
                           title, path);
}

static void print_media(GDBusProxy *proxy, const char *description)
{
    char *str;

    str = proxy_description(proxy, "Media", description);

    bt_shell_printf("%s\n", str);
    print_property(proxy, "SupportedUUIDs");

    g_free(str);
}

static void print_player(void *data, void *user_data)
{
    GDBusProxy *proxy = data;
    const char *description = user_data;
    char *str;

    str = proxy_description(proxy, "Player", description);

    bt_shell_printf("%s%s\n", str,
                    default_player == proxy ? "[default]" : "");

    g_free(str);
}

static void cmd_list(int argc, char *arg[])
{
    g_list_foreach(players, print_player, NULL);

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_show(int argc, char *argv[])
{
    GDBusProxy *proxy;
    DBusMessageIter iter;
    const char *path;

    if (argc < 2)
    {
        if (check_default_player() == FALSE)
            return bt_shell_noninteractive_quit(EXIT_FAILURE);

        proxy = default_player;
    }
    else
    {
        proxy = g_dbus_proxy_lookup(players, NULL, argv[1],
                                    BLUEZ_MEDIA_PLAYER_INTERFACE);
        if (!proxy)
        {
            bt_shell_printf("Player %s not available\n", argv[1]);
            return bt_shell_noninteractive_quit(EXIT_FAILURE);
        }
    }

    bt_shell_printf("Player %s\n", g_dbus_proxy_get_path(proxy));

    print_property(proxy, "Name");
    print_property(proxy, "Repeat");
    print_property(proxy, "Equalizer");
    print_property(proxy, "Shuffle");
    print_property(proxy, "Scan");
    print_property(proxy, "Status");
    print_property(proxy, "Position");
    print_property(proxy, "Track");

    dbus_message_iter_get_basic(&iter, &path);


    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_select(int argc, char *argv[])
{
    GDBusProxy *proxy;

    proxy = g_dbus_proxy_lookup(players, NULL, argv[1],
                                BLUEZ_MEDIA_PLAYER_INTERFACE);
    if (proxy == NULL)
    {
        bt_shell_printf("Player %s not available\n", argv[1]);
        return bt_shell_noninteractive_quit(EXIT_FAILURE);
    }

    if (default_player == proxy)
        return bt_shell_noninteractive_quit(EXIT_SUCCESS);

    default_player = proxy;
    print_player(proxy, NULL);

    return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static const struct bt_shell_menu player_menu = {
    .name = "player",
    .desc = "Media Player Submenu",
    .entries = {
        {"list", NULL, cmd_list, "List available players"},
        {"show", "[player]", cmd_show, "Player information",
         player_generator},
        {"select", "<player>", cmd_select, "Select default player",
         player_generator},
        {"play", NULL, cmd_play, "Start playback"},
        {"pause", NULL, cmd_pause, "Pause playback"},
        {"stop", NULL, cmd_stop, "Stop playback"},
        {"next", NULL, cmd_next, "Jump to next item"},
        {"previous", NULL, cmd_previous, "Jump to previous item"},
        {"fast-forward", NULL, cmd_fast_forward,
         "Fast forward playback"},
        {"rewind", NULL, cmd_rewind, "Rewind playback"},
        {"equalizer", "<on/off>", cmd_equalizer,
         "Enable/Disable equalizer"},
        {"repeat", "<singletrack/alltrack/group/off>", cmd_repeat,
         "Set repeat mode"},
        {"shuffle", "<alltracks/group/off>", cmd_shuffle,
         "Set shuffle mode"},
        {}},
};

static void media_added(GDBusProxy *proxy)
{
    medias = g_list_append(medias, proxy);

    print_media(proxy, COLORED_NEW);
}

static void player_added(GDBusProxy *proxy)
{
    players = g_list_append(players, proxy);

    if (default_player == NULL)
        default_player = proxy;

    print_player(proxy, COLORED_NEW);
}

static void proxy_added(GDBusProxy *proxy, void *user_data)
{
    const char *interface;

    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, BLUEZ_MEDIA_INTERFACE))
        media_added(proxy);
    else if (!strcmp(interface, BLUEZ_MEDIA_PLAYER_INTERFACE))
        player_added(proxy);
}

static void media_removed(GDBusProxy *proxy)
{
    print_media(proxy, COLORED_DEL);

    medias = g_list_remove(medias, proxy);
}

static void player_removed(GDBusProxy *proxy)
{
    print_player(proxy, COLORED_DEL);

    if (default_player == proxy)
        default_player = NULL;

    players = g_list_remove(players, proxy);
}

static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
    const char *interface;

    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, BLUEZ_MEDIA_INTERFACE))
        media_removed(proxy);
    if (!strcmp(interface, BLUEZ_MEDIA_PLAYER_INTERFACE))
        player_removed(proxy);
}

static void player_property_changed(GDBusProxy *proxy, const char *name,
                                    DBusMessageIter *iter)
{
    char *str;

    str = proxy_description(proxy, "Player", COLORED_CHG);
    print_iter(str, name, iter);
    g_free(str);
}

static void property_changed(GDBusProxy *proxy, const char *name,
                             DBusMessageIter *iter, void *user_data)
{
    const char *interface;

    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, BLUEZ_MEDIA_PLAYER_INTERFACE))
        player_property_changed(proxy, name, iter);
}

static GDBusClient *client;

void player_add_submenu(void)
{
    bt_shell_add_submenu(&player_menu);

    dbus_conn = bt_shell_get_env("DBUS_CONNECTION");
    if (!dbus_conn || client)
        return;

    client = g_dbus_client_new(dbus_conn, "org.bluez", "/org/bluez");

    g_dbus_client_set_proxy_handlers(client, proxy_added, proxy_removed,
                                     property_changed, NULL);
}

void player_remove_submenu(void)
{
    g_dbus_client_unref(client);
}