#define _POSIX_C_SOURCE 200809L
#include "app/init.h"
#include "app/app_model.h"
#include "hal/rotary_encoder.h"
#include "hal/draw_stuff.h"
#include "hal/joystick.h"
#include "hal/lg_gpio_samples_func.h"
#include "hal/audio_capture.h"
#include "hal/microphone.h"
#include "ui/load_image_assets.h"
#include "hal/bt_agent.h"
#include "hal/bt_dbus.h"
#include "hal/bt_player.h"

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

static _Atomic bool shutdown = false;
static pthread_barrier_t barrier;
// pthread_barrier_t with count = 1 not allowed
static bool num_confirms_0 = false;


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


int init_start(int num_confirms)
{
    int code;

    code = lg_gpio_samples_func_init();
    if(code) {
        fprintf(stderr, "init: failed to init lg_gpio_samples_func %d\n", code);
        return 0;
    }

    code = load_image_assets_init();
    if(code) {
        fprintf(stderr, "init: failed to load image assets %d\n", code);
        return 1;
    }

    code = joystick_init();
    if(code) {
        fprintf(stderr, "failed to init stick %d\n", code);
        return 2;
    }

    code = rotary_encoder_init();
    if(code) {
        fprintf(stderr, "failed to init rotary encoder %d\n", code);
        return 3;
    }


    draw_stuff_init();

    if(num_confirms > 0) {
        code = pthread_barrier_init(&barrier, NULL, num_confirms + 1);
        if(code) {
            fprintf(stderr, "failed to init barrier %d\n", code);
            return 5;
        }
        num_confirms_0 = false;
    } else {
        num_confirms_0 = true;
    }


    bt_agent_init();

    dbus_init();

    bt_player_init();

    app_model_init();

    // ENABLE FOR MICROPHONE
    // audio_capture_init();
    // microphone_init();
    
    return 0;
}

void init_set_shutdown()
{
    shutdown = true;
}

bool init_get_shutdown()
{
    return shutdown;
}

void init_signal_done()
{
    if(num_confirms_0) {
        return;
    }
    int code = pthread_barrier_wait(&barrier);
    if(code != PTHREAD_BARRIER_SERIAL_THREAD && code != 0) {
        fprintf(stderr, "failed to wait barrier init_signal_done %d\n", code);
    }
}

void init_end(void) 
{
    int code;
    if(!num_confirms_0) {
        code = pthread_barrier_wait(&barrier);
        if(code != PTHREAD_BARRIER_SERIAL_THREAD && code != 0) {
            fprintf(stderr, "failed to wait barrier init_end %d\n", code);
        }
    }

    app_model_cleanup();

    bt_player_cleanup();

    dbus_cleanup();

    bt_agent_cleanup();
    draw_stuff_cleanup();

    rotary_encoder_cleanup();
    if(!num_confirms_0) {
        code = pthread_barrier_destroy(&barrier);
        if(code) {
            fprintf(stderr, "failed to destroy barrier %d\n", code);
        }
    }

    joystick_cleanup();

    // ENABLE FOR MICROPHONE
    // audio_capture_cleanup();
    // microphone_cleanup();
    
    load_image_assets_cleanup();
    lg_gpio_samples_func_cleanup();
}