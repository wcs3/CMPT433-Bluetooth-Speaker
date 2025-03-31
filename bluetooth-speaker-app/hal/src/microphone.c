#include "hal/microphone.h"
#include "hal/audio_capture.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_INPUT 2048
#define SAMPLE_RATE 16000
#define SAMPLE_SIZE (sizeof(short))

static char audio_input_char[MAX_INPUT]; // Stores the raw input from the microphone
static short* audio_input_samples = NULL;
static size_t num_samples = 0;
static bool is_initialized = false;
static bool is_running = false;
static pthread_t mic_thread;

static void* listen_for_audio(void* arg) {
    (void)arg;

    while (is_running) {
        //TODO NEED TO PERFORM SPEECH TO TEXT TRANSFORMATION

        // Capture the audio samples from ALSA
        audio_capture_get_audio_buffer(&audio_input_samples, &num_samples);
        
        sleep(1);
    }

    return NULL;
}

void microphone_init(void) {
    // TODO
    is_initialized = true;
}

void microphone_enable_audio_listening(void) {
    assert(is_initialized);
    is_running = true;

    // Start the ALSA thread to capture data
    audio_capture_start();

    // Start a background thread to listen for audio input
    pthread_create(&mic_thread, NULL, listen_for_audio, NULL);
}

void microphone_disable_audio_listening(void) {
    assert(is_initialized);

    // Stop the ALSA audio capture
    audio_capture_stop();
}

const char* microphone_get_audio_input(void) {
    // MIGHT NOT NEED THIS FUNCTION IN THE FUTURE
    return audio_input_char;
}

enum keyword microphone_get_keyword_from_audio_input(const char* audio_input) {
    assert(is_initialized);

    if (strstr(audio_input, "stop")) {
        return STOP;
    } else if (strstr(audio_input, "next song")) {
        return NEXT;
    } else if (strstr(audio_input, "previous")) {
        return PREVIOUS;
    }else if (strstr(audio_input, "volume up")) {
        return VOLUME_UP;
    } else if (strstr(audio_input, "volume down")) {
        return VOLUME_DOWN;
    } else {
        return KEYWORD_NONE;
    }
}

void microphone_cleanup(void) {
    // TODO
    assert(is_initialized);
    is_initialized = false;
    is_running = false;
    free(audio_input_samples);
    audio_input_samples = NULL;

    // Cleans up the background thread
    pthread_join(mic_thread, NULL);
}


