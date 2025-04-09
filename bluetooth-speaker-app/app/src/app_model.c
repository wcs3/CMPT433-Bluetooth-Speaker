#include "app/app_model.h"
#include "hal/bt_player.h"
#include "hal/time_util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

static int volume = 80;

static uint32_t position = 0;
static long position_changed_at = 0;
static bool is_playing = false;
static bt_player_shuffle_e shuffle = BT_PLAYER_SHUFFLE_OFF;
static bt_player_repeat_e repeat = BT_PLAYER_REPEAT_OFF;

void on_position_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    position = *(const uint32_t *)changed_val;

    position_changed_at = time_ms();
}

void on_status_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    is_playing = *(const bt_player_status_e *)changed_val == BT_PLAYER_STATUS_PLAYING;
    printf("status changed: %s\n", is_playing ? "playing" : "paused");
}

void on_volume_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    volume = *(const uint8_t *)changed_val;
    printf("volume changed: %d\n", volume);
}

void on_shuffle_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    shuffle = *(const bt_player_shuffle_e *)changed_val;
    printf("shuffle changed: %d\n", shuffle);
}

void on_repeat_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    repeat = *(const bt_player_repeat_e *)changed_val;
    printf("repeat changed: %d\n", repeat);
}

int app_model_init()
{
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_PLAYBACK_POSITION, on_position_changed, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_PLAYBACK_STATUS, on_status_changed, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_VOLUME, on_volume_changed, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_SHUFFLE_MODE, on_shuffle_changed, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_REPEAT_MODE, on_repeat_changed, NULL);
    return 0;
}

void app_model_cleanup()
{
    return;
}

char *app_model_get_track_title()
{
    bt_player_track_info_t track;
    if (!bt_player_get_property(BT_PLAYER_PROP_TRACK, &track))
        return strdup("N/A");

    free(track.album);
    free(track.artist);
    free(track.genre);

    if (!track.title || !strcmp(track.title, ""))
    {
        free(track.title);
        return strdup("N/A");
    }

    return track.title;
}

char *app_model_get_album_title()
{
    bt_player_track_info_t track;
    if (!bt_player_get_property(BT_PLAYER_PROP_TRACK, &track))
        return strdup("N/A");

    free(track.title);
    free(track.artist);
    free(track.genre);

    if (!track.album || !strcmp(track.album, ""))
    {
        free(track.album);
        return strdup("N/A");
    }

    return track.album;
}

char *app_model_get_genre()
{
    bt_player_track_info_t track;
    if (!bt_player_get_property(BT_PLAYER_PROP_TRACK, &track))
        return strdup("N/A");

    free(track.title);
    free(track.artist);
    free(track.album);

    if (!track.genre || !strcmp(track.genre, ""))
    {
        free(track.genre);
        return strdup("N/A");
    }

    return track.genre;
}

char *app_model_get_artist()
{
    bt_player_track_info_t track;
    if (!bt_player_get_property(BT_PLAYER_PROP_TRACK, &track))
        return strdup("N/A");

    free(track.title);
    free(track.genre);
    free(track.album);

    if (!track.artist || !strcmp(track.artist, ""))
    {
        free(track.artist);
        return strdup("N/A");
    }

    return track.artist;
}

int app_model_get_volume()
{
    return volume * 100.0 / 127;
}

app_state_playback app_model_get_playback()
{
    bt_player_track_info_t track;
    if (!bt_player_get_property(BT_PLAYER_PROP_TRACK, &track))
        return (app_state_playback){0};

    free(track.title);
    free(track.genre);
    free(track.album);
    free(track.artist);

    if (!is_playing)
    {
        return (app_state_playback){
            .seconds_passed = position / 1000,
            .seconds_total = track.duration / 1000,
        };
    }

    long curr_pos = position + (time_ms() - position_changed_at);
    return (app_state_playback){
        .seconds_passed = curr_pos / 1000,
        .seconds_total = track.duration / 1000,
    };
}

int app_model_get_shuffle()
{
    return shuffle;
}

int app_model_get_repeat()
{
    return repeat;
}

bool app_model_is_playing()
{
    return is_playing;
}

int app_model_toggle_pause_play()
{
    if (is_playing)
        return !bt_player_send_command(BT_PLAYER_CMD_PAUSE);
    else
        return !bt_player_send_command(BT_PLAYER_CMD_PLAY);
}

int app_model_pause()
{
    return !bt_player_send_command(BT_PLAYER_CMD_PAUSE);
}

int app_model_next()
{
    return !bt_player_send_command(BT_PLAYER_CMD_NEXT);
}

int app_model_previous()
{
    position = 0;
    position_changed_at = time_ms();
    return !bt_player_send_command(BT_PLAYER_CMD_PREV);
}

int app_model_toggle_shuffle()
{
    int ret = 1;

    bt_player_shuffle_e new_shuffle = (shuffle + 1) % BT_PLAYER_SHUFFLE_cnt;
    if (bt_player_set_property(BT_PLAYER_PROP_SHUFFLE_MODE, &new_shuffle))
        ret = 0;

    return ret;
}

int app_model_toggle_repeat()
{
    int ret = 1;

    bt_player_repeat_e new_repeat = (repeat + 1) % BT_PLAYER_REPEAT_cnt;
    if (bt_player_set_property(BT_PLAYER_PROP_REPEAT_MODE, &new_repeat))
        ret = 0;

    return ret;
}

int app_model_increase_volume()
{
    int new_volume = volume + 5;
    if (new_volume > 127)
        new_volume = 127;

    return !bt_player_set_property(BT_PLAYER_PROP_VOLUME, &new_volume);
}

int app_model_decrease_volume()
{
    int new_volume = volume - 5;
    if (new_volume < 0)
        new_volume = 0;

    return !bt_player_set_property(BT_PLAYER_PROP_VOLUME, &new_volume);
}