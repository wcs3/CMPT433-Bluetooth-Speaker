#include "app/app_model.h"
#include "hal/bt_player.h"

#include <stdlib.h>
#include <string.h>

static int volume = 80;

int app_model_init() 
{
    return 0;
}

void app_model_cleanup() 
{
    return;
}

char* app_model_get_track_title() 
{
    char* res = malloc(64);
    strncpy(res, "Title", 64);
    return res;
}

char* app_model_get_album_title() 
{
    char* res = malloc(64);
    strncpy(res, "Albumabcdefghijklmnopqrstuvwxyz", 64);
    return res;
}

char* app_model_get_genre() 
{
    char* res = malloc(64);
    strncpy(res, "Genre", 64);
    return res;
}

char* app_model_get_artist() 
{
    char* res = malloc(64);
    strncpy(res, "Artist", 64);
    return res;
}

int app_model_get_volume() 
{
    return 0;
}

app_state_playback app_model_get_playback() 
{
    app_state_playback state = {100, 200};
    return state;
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