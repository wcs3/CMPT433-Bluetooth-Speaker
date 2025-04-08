// handles terminal output, LCD, rotary encoder controls
#include "app/user_interface.h"
#include "app/app_model.h"
#include "app/init.h"
#include "hal/rotary_encoder.h"
#include "hal/draw_stuff.h"
#include "hal/image_loader.h"
#include "ui/draw_ui.h"
#include "ui/load_image_assets.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_t ui_thread;

void* run_ui(void* arg __attribute__((unused)))
{
    while(!init_get_shutdown()) {

        // char* track_name =

        Olivec_Canvas* screen = image_loader_image_create(240, 240);
        Olivec_Canvas* album = draw_ui_text("Album Name");
        Olivec_Canvas* track = draw_ui_text("Track Name");
        Olivec_Canvas* time_txt = draw_ui_text("0:11 / 2:11");
        Olivec_Canvas* time_bar = draw_ui_progress_bar(120, 5, 0.89f, OLIVEC_RGBA(255, 0, 0, 255));
        Olivec_Canvas* volume_bar = draw_ui_progress_bar(120, 5, 0.5f, OLIVEC_RGBA(0, 0, 255, 255));
        Olivec_Canvas* volume_icon = load_image_assets_get_volume_icon();

        sleep(1);
    }

    init_signal_done();
    return NULL;
}

int user_interface_init()
{
    rotary_encoder_set_turn_listener(NULL);
    int thread_code = pthread_create(&ui_thread, NULL, run_ui, NULL);
    if(thread_code) {
        fprintf(stderr, "failed to create ui thread %d\n", thread_code);
        return 1;
    }
    return 0;
}

void user_interface_cleanup()
{
    int thread_code = pthread_join(ui_thread, NULL);
    if(thread_code) {
        fprintf(stderr, "failed to join ui thread %d\n", thread_code);
    }
}