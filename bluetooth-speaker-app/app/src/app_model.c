#include "app/app_model.h"

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
    return "";
}

char* app_model_get_album_title() 
{
    return "";
}

char* app_model_get_genre() 
{
    return "";
}

char* app_model_get_artist() 
{
    return "";
}

int app_model_get_volume() 
{
    return 0;
}

app_state_playback app_model_get_playback() 
{
    app_state_playback state = {0, 0};
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