#include "hal/microphone.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_INPUT 2048

static char audio_input[MAX_INPUT]; // Stores the raw input from the microphone
static bool is_mic_initialized = false;
static bool is_mic_enabled = false;
static pthread_t mic_thread;

static void* listen_for_audio(void* arg) {
    (void)arg;

    while (is_mic_enabled) {
        //TODO NEED TO PERFORM SPEECH TO TEXT TRANSFORMATION
        sleep(1);
    }

    return NULL;
}

void microphone_init(void) {
    // TODO
    is_mic_initialized = true;
}

void microphone_enable_audio_listening(void) {
    assert(is_mic_initialized);
    assert(is_mic_enabled);
    is_mic_enabled = true;

    // Start a background thread to listen for audio input
    pthread_create(&mic_thread, NULL, listen_for_audio, NULL);
}

void microphone_disable_audio_listening(void) {
    assert(is_mic_enabled);
    assert(is_mic_initialized);
    is_mic_enabled = false;

    // Cleans up the background thread
    pthread_join(mic_thread, NULL);
}

const char* microphone_get_audio_input(void) {
    // MIGHT NOT NEED THIS FUNCTION IN THE FUTURE
    return audio_input;
}

enum keyword microphone_get_keyword_from_audio_input(const char* audio_input) {
    assert(is_mic_initialized);

    if (strstr(audio_input, "stop")) {
        return STOP;
    } else if (strstr(audio_input, "next song")) {
        return NEXT;
    } else if (strstr(audio_input, "volume up")) {
        return VOLUME_UP;
    } else if (strstr(audio_input, "volume down")) {
        return VOLUME_DOWN;
    } else {
        return KEYWORD_NONE;
    }
}

void microphone_cleanup(void) {
    // TODO
    assert(is_mic_initialized);
    assert(!is_mic_enabled);
    is_mic_initialized = false;
}


