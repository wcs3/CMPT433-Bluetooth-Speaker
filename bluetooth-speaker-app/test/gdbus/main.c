#include "bt_dbus.h"
#include "bt_player.h"

static bool done = false;

void cleanup_handler(int signum, siginfo_t *info, void *arg)
{
    (void)signum;
    (void)info;
    (void)arg;
    bt_player_cleanup();
    dbus_cleanup();

    done = true;
}

void on_track_change(const void *val, void *user_data)
{
    (void)user_data;

    g_print("\ntrack changed callback:\n");
    const bt_player_track_info_t *new_track = val;
    g_print("\tTitle: %s\n", new_track->title);
    g_print("\tArtist: %s\n", new_track->artist);
    g_print("\tAlbum: %s\n", new_track->album);
    g_print("\tGenre: %s\n", new_track->genre);
    uint32_t dur_min = (new_track->duration / 1000) / 60;
    uint32_t dur_sec = (new_track->duration / 1000) % 60;
    g_print("\tDuration: %u:%02u\n\n", dur_min, dur_sec);
}

void on_position_change(const void *val, void *user_data)
{
    (void)user_data;
    const uint32_t *new_pos = val;
    uint32_t pos_min = (*new_pos / 1000) / 60;
    uint32_t pos_sec = (*new_pos / 1000) % 60;
    uint32_t pos_ms = (*new_pos / 100) % 10;
    g_print("\nposition changed callback: %u:%02u.%u\n\n", pos_min, pos_sec, pos_ms);
}

int main()
{
    g_print("Starting gbus_test\n");

    struct sigaction action = {0};
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = cleanup_handler;
    sigaction(SIGINT, &action, NULL);

    dbus_init();
    g_print("dbus initialized\n");
    bt_player_init();
    g_print("bt_player initialized\n");

    bt_player_set_property_changed_cb(BT_PLAYER_PROP_TRACK, on_track_change, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_PLAYBACK_POSITION, on_position_change, NULL);

    sleep(5);

    bt_player_send_command(BT_PLAYER_CMD_PLAY);

    sleep(3);

    bt_player_send_command(BT_PLAYER_CMD_PAUSE);

    sleep(3);

    bt_player_send_command(BT_PLAYER_CMD_PLAY);

    sleep(3);

    bt_player_send_command(BT_PLAYER_CMD_NEXT);

    sleep(3);

    bt_player_send_command(BT_PLAYER_CMD_PREV);

    sleep(3);

    uint8_t vol = 127;
    bt_player_set_property(BT_PLAYER_PROP_VOLUME, &vol);

    sleep(3);

    vol = 0;
    bt_player_set_property(BT_PLAYER_PROP_VOLUME, &vol);

    sleep(3);

    vol = 50;
    bt_player_set_property(BT_PLAYER_PROP_VOLUME, &vol);

    while (!done)
    {
        sleep(1);
    }

    return 0;
}