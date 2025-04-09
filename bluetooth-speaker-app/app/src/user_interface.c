// handles terminal output, LCD, rotary encoder controls
#include "app/user_interface.h"
#include "app/app_model.h"
#include "app/init.h"
#include "hal/rotary_encoder.h"
#include "hal/draw_stuff.h"
#include "hal/image_loader.h"
#include "ui/draw_ui.h"
#include "ui/load_image_assets.h"
#include "hal/time_util.h"
#include "hal/joystick.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static pthread_t ui_thread;

// // max_size is the max number of characters you want displayed
// static void trim_string(char* src, char* dest, int max_size)
// {
//     strncpy(dest, src, max_size);
//     dest[max_size] = '\0';
// }

void* run_ui(void* arg __attribute__((unused)))
{
    int max_chars = LCD_WIDTH / LOAD_IMAGE_ASSETS_CHAR_WIDTH;
    char* album_str_buf = malloc(sizeof(*album_str_buf) * (max_chars + 1));
    char* track_str_buf = malloc(sizeof(*track_str_buf) * (max_chars + 1));
    char* playback_str_buf = malloc(sizeof(*playback_str_buf) * (max_chars + 1));
    char* artist_str_buf = malloc(sizeof(*artist_str_buf) * (max_chars + 1));
    Olivec_Canvas* screen = image_loader_image_create(240, 240);

    while(!init_get_shutdown()) {
        char* album_temp = app_model_get_album_title();
        char* track_temp = app_model_get_track_title();
        char* track_artist = app_model_get_artist();
        app_state_playback playback = app_model_get_playback();
        int volume = app_model_get_volume();
        int shuffle = app_model_get_shuffle();
        int repeat = app_model_get_repeat();
        bool playing = app_model_is_playing();

        // stop compiler from complaining about string getting cut off because we actually want that
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-truncation"

        snprintf(album_str_buf, max_chars + 1, "%s", album_temp);
        snprintf(track_str_buf, max_chars + 1, "%s", track_temp);
        snprintf(artist_str_buf, max_chars + 1, "By: %s", track_artist);
        snprintf(playback_str_buf, max_chars + 1, "%02d:%02d / %02d:%02d", 
            playback.seconds_passed / 60, 
            playback.seconds_passed % 60, 
            playback.seconds_total / 60, 
            playback.seconds_total % 60
        );

        #pragma GCC diagnostic pop


        free(album_temp);
        free(track_temp);
        free(track_artist);
        
        Olivec_Canvas* album_txt = draw_ui_text(album_str_buf);
        Olivec_Canvas* track_txt = draw_ui_text(track_str_buf);
        Olivec_Canvas* artist_txt = draw_ui_text(artist_str_buf);
        Olivec_Canvas* time_txt = draw_ui_text(playback_str_buf);
        Olivec_Canvas* time_bar = draw_ui_progress_bar(120, 5, playback.seconds_passed / (float) playback.seconds_total, OLIVEC_RGBA(255, 0, 0, 255));
        Olivec_Canvas* volume_bar = draw_ui_progress_bar(120, 5, volume / 100.0f, OLIVEC_RGBA(0, 0, 255, 255));
        Olivec_Canvas* volume_icon = load_image_assets_get_volume_icon();

        int mid_section_start = 60;

        olivec_fill(*screen, OLIVEC_RGBA(255, 255, 255, 255));
        draw_ui_blend_centered(*screen, *album_txt, 10);
        draw_ui_blend_centered(*screen, *track_txt, mid_section_start);
        draw_ui_blend_centered(*screen, *artist_txt, mid_section_start + 20);
        draw_ui_blend_centered(*screen, *time_bar, mid_section_start + 45);
        draw_ui_blend_centered(*screen, *time_txt, mid_section_start + 50);

        int vol_bar_x = draw_ui_blend_centered(*screen, *volume_bar, mid_section_start + 160);
        olivec_sprite_blend(*screen, vol_bar_x - 25, mid_section_start + 153, volume_icon->width, volume_icon->height, *volume_icon);

        if (shuffle == 1)
        {
            Olivec_Canvas *shuffle_icon = load_image_assets_get_shuffle_icon();
            olivec_sprite_blend(*screen, 50, mid_section_start + 80, shuffle_icon->width, shuffle_icon->height, *shuffle_icon);
        }

        if (repeat == 1)
        {
            Olivec_Canvas *replay_icon = load_image_assets_get_replay_icon();
            olivec_sprite_blend(*screen, 160, mid_section_start + 80, replay_icon->width, replay_icon->height, *replay_icon);
        }
        else if (repeat == 2)
        {
            Olivec_Canvas *repeat_icon = load_image_assets_get_repeat_icon();
            olivec_sprite_blend(*screen, 160, mid_section_start + 80, repeat_icon->width, repeat_icon->height, *repeat_icon);
        }

        if (playing)
        {
            Olivec_Canvas *pause_icon = load_image_assets_get_pause_icon();
            draw_ui_blend_centered(*screen, *pause_icon, mid_section_start + 80);
        }
        else
        {
            Olivec_Canvas *play_icon = load_image_assets_get_play_icon();
            draw_ui_blend_centered(*screen, *play_icon, mid_section_start + 80);
        }

        draw_stuff_screen(screen);

        image_loader_image_free(&album_txt);
        image_loader_image_free(&track_txt);
        image_loader_image_free(&time_txt);
        image_loader_image_free(&time_bar);
        image_loader_image_free(&volume_bar);
        image_loader_image_free(&artist_txt);
    }

    free(album_str_buf);
    free(track_str_buf);
    free(playback_str_buf);
    free(artist_str_buf);
    image_loader_image_free(&screen);

    init_signal_done();
    return NULL;
}

void listen_prev()
{
    printf("previous\n");
    int code = app_model_previous();
    if (code)
    {
        fprintf(stderr, "user_interface: app_model_previous failed %d\n", code);
    }
}

void listen_next()
{
    printf("next\n");
    int code = app_model_next();
    if (code)
    {
        fprintf(stderr, "user_interface: app_model_next failed %d\n", code);
    }
}

void listen_pause_play()
{
    printf("toggle pause/play\n");
    int code = app_model_toggle_pause_play();
    if (code)
    {
        fprintf(stderr, "user_interface: app_model_toggle_pause_play failed %d\n", code);
    }
}

void listen_shuffle()
{
    printf("cycle shuffle mode\n");
    int code = app_model_toggle_shuffle();
    if (code)
    {
        fprintf(stderr, "user_interface: app_model_toggle_shuffle failed %d\n", code);
    }
}

void listen_repeat()
{
    printf("cycle repeat mode\n");
    int code = app_model_toggle_repeat();
    if (code)
    {
        fprintf(stderr, "user_interface: app_model_toggle_repeat failed %d\n", code);
    }
}

void on_encoder_turn(bool clockwise)
{
    if (clockwise)
    {
        app_model_increase_volume();
        // printf("Rotated Clockwise!\n");
    }
    else
    {
        app_model_decrease_volume();
        // printf("Rotated Counter-Clockwise!\n");
    }
}

int user_interface_init()
{
    rotary_encoder_set_turn_listener(on_encoder_turn);
    joystick_set_on_press_listener(listen_pause_play);
    joystick_set_on_up_listener(listen_shuffle);  // listen_play
    joystick_set_on_down_listener(listen_repeat); // listen_pause
    joystick_set_on_left_listener(listen_prev);   // listen_up
    joystick_set_on_right_listener(listen_next);  // listen_next
    int thread_code = pthread_create(&ui_thread, NULL, run_ui, NULL);
    if (thread_code)
    {
        fprintf(stderr, "failed to create ui thread %d\n", thread_code);
        return 1;
    }
    return 0;
}

void user_interface_cleanup()
{
    int thread_code = pthread_join(ui_thread, NULL);
    if (thread_code)
    {
        fprintf(stderr, "failed to join ui thread %d\n", thread_code);
    }
}