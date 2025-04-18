// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "hal/rotary_encoder.h"
#include "hal/draw_stuff.h"
#include "hal/microphone.h"
#include "hal/audio_capture.h"
#include "hal/bt_agent.h"

#include "app/init.h"
#include "app/user_interface.h"

static const int NUM_APP_THREADS = 1;

void ctrl_c_handler(int signum __attribute__((unused))) {
    init_set_shutdown();
}

int main()
{
    if(init_start(NUM_APP_THREADS)) {
        return 1;
    }

    // set up signal handler to close gracefully on ctrl-c
    struct sigaction sa;
    sa.sa_handler = ctrl_c_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // functions that start other application threads
    user_interface_init();

    init_end();

    user_interface_cleanup();

    return 0;
}
