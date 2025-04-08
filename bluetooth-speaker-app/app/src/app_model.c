#include "app/app_model.h"

int app_model_init() 
{
    return 0;
}

void app_model_cleanup() 
{
    return;
}

char* get_track_title() 
{
    return "";
}

char* get_album_title() 
{
    return "";
}

char* get_genre() 
{
    return "";
}

char* get_artist() 
{
    return "";
}

int get_volume() 
{
    return 0;
}

app_state_playback get_playback() 
{
    app_state_playback state = {0, 0};
    return state;
}

int play() 
{
    return 0;
}

int pause() 
{
    return 0;
}

int next() 
{
    return 0;
}

int previous() 
{
    return 0;
}

int shuffle() 
{
    return 0;
}

int repeat() 
{
    return 0;
}