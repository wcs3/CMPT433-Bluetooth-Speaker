
#ifndef _APP_MODEL_H
#define _APP_MODEL_H

// a model for the bluetooth speaker, contains getters for state and functions for sending commands to phone
// implement these so that they can be called at arbitrary times after init

int app_model_init();
void app_model_cleanup();

typedef struct {
    int seconds_passed;
    int seconds_total;
} app_state_playback;

// assume the char* returns of all of these are immutable c strings
// if nothing is set just return some sensible defaults (ie empty string, 00:00 time, 0 volume)
char* app_model_get_track_title();

char* app_model_get_album_title();

char* app_model_get_genre();

char* app_model_get_artist();

// return a number from 0 to 100
int app_model_get_volume();

app_state_playback app_model_get_playback();

// commands the board can send, these are going to be needed for both voice commands and controls on the board so they should be thread safe

int app_model_play();

int app_model_pause();

int app_model_next();

int app_model_previous();

int app_model_shuffle();

int app_model_repeat();

void app_model_increase_volume();

void app_model_decrease_volume();

#endif