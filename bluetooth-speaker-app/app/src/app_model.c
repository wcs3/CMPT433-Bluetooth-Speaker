#include "app/app_model.h"
#include <stdlib.h>
#include <string.h>

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
    return 0;
}

int app_model_pause() 
{
    return 0;
}

int app_model_next() 
{
    return 0;
}

int app_model_previous() 
{
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