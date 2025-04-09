#include "app/app_model.h"
#include "hal/bt_player.h"
#include "hal/time_util.h"

#include <stdlib.h>
#include <string.h>

static int volume = 80;

static bt_player_track_info_t track;
static uint32_t position = 0;
static long position_changed_at = 0;
static bool is_playing = false;

void on_track_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    const bt_player_track_info_t *new_track = changed_val;

    free(track.title);
    free(track.artist);
    free(track.album);
    free(track.genre);

    track.title = strdup(new_track->title);
    track.album = strdup(new_track->album);
    track.artist = strdup(new_track->artist);
    track.genre = strdup(new_track->genre);
    track.duration = new_track->duration;
}

void on_position_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    position = *(const uint32_t *)changed_val;
    uint32_t pos_min = (position / 1000) / 60;
    uint32_t pos_sec = (position / 1000) % 60;
    uint32_t pos_ms = (position / 100) % 10;
    g_print("\nposition changed callback: %u:%02u.%u\n\n", pos_min, pos_sec, pos_ms);

    position_changed_at = time_ms();
}

void on_status_changed(const void *changed_val, void *user_data)
{
    (void)user_data;

    is_playing = *(const bt_player_status_e *)changed_val == BT_PLAYER_STATUS_PLAYING;
}

void on_volume_changed(const void *changed_val, void *user_data) {
    (void) user_data;

}

int app_model_init()
{
    track = (bt_player_track_info_t){0};
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_TRACK, on_track_changed, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_PLAYBACK_POSITION, on_position_changed, NULL);
    bt_player_set_property_changed_cb(BT_PLAYER_PROP_PLAYBACK_STATUS, on_status_changed, NULL);
    return 0;
}

void app_model_cleanup()
{
    free(track.title);
    free(track.artist);
    free(track.album);
    free(track.genre);
    return;
}

char *app_model_get_track_title()
{
    if (!track.title || !strcmp(track.title, ""))
        return strdup("N/A");
    return strdup(track.title);
}

char *app_model_get_album_title()
{
    if (!track.album || !strcmp(track.album, ""))
        return strdup("N/A");
    return strdup(track.album);
}

char *app_model_get_genre()
{
    if (!track.genre || !strcmp(track.genre, ""))
        return strdup("N/A");
    return strdup(track.genre);
}

char *app_model_get_artist()
{
    if (!track.artist || !strcmp(track.artist, ""))
        return strdup("N/A");
    return strdup(track.artist);
}

int app_model_get_volume()
{
    return 0;
}

app_state_playback app_model_get_playback()
{
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

int app_model_play()
{
    bt_player_send_command(BT_PLAYER_CMD_PLAY);
    return 0;
}

int app_model_pause()
{
    bt_player_send_command(BT_PLAYER_CMD_PAUSE);
    return 0;
}

int app_model_next()
{
    bt_player_send_command(BT_PLAYER_CMD_NEXT);
    return 0;
}

int app_model_previous()
{
    bt_player_send_command(BT_PLAYER_CMD_PREV);
    position = 0;
    position_changed_at = time_ms();
    return 0;
}

int app_model_shuffle()
{
    return 0;
}

int app_model_repeat()
{
    return 0;
}

void app_model_increase_volume(){
    volume += 5;
    if (volume > 120){
        volume = 100;
    }
    bt_player_set_property(BT_PLAYER_PROP_VOLUME, &volume);
}


void app_model_decrease_volume(){
    volume -= 5;
    if (volume < 0){
        volume = 0;
    }
    bt_player_set_property(BT_PLAYER_PROP_VOLUME, &volume);
}