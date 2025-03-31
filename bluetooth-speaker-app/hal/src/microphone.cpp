#include "hal/microphone.h"
#include "hal/audio_capture.h"
#include "hal/whisper.h"

#include <cassert>
#include <pthread.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <cstdlib> 

#define MAX_INPUT 2048
#define SAMPLE_RATE 16000
#define SAMPLE_SIZE (sizeof(short))

static char audio_input_char[MAX_INPUT]; // Stores the raw input from the microphone
static short* audio_input_samples = nullptr;
static size_t num_samples = 0;
static bool is_initialized = false;
static bool is_running = false;
static pthread_t mic_thread;

static void* listen_for_audio(void* arg) {
    (void)arg;

    // Load Whisper model
    struct whisper_context* ctx = whisper_init_from_file("/mnt/remote/models/ggml-base.en.bin");
    if (!ctx) {
        fprintf(stderr, "Failed to initialize whisper context\n");
        return nullptr;
    }

    // Set default params for transcription
    struct whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.print_special = false;

    while (is_running) {
        // Capture the audio samples from ALSA
        audio_capture_get_audio_buffer(&audio_input_samples, &num_samples);

        // Transcribe audio with Whisper
        if (whisper_full(ctx, wparams, audio_input_samples, num_samples) != 0) {
            fprintf(stderr, "Whisper transcription failed\n");
            continue;
        }

        // Get the transcribed result
        const char* result = whisper_full_get_segment_text(ctx, 0);
        if (result) {
            std::strncpy(audio_input_char, result, MAX_INPUT);
            audio_input_char[MAX_INPUT - 1] = '\0';  // Ensure null-termination
        }
    }

    whisper_free(ctx);
    return nullptr;
}

void microphone_init(void) {
    is_initialized = true;
}

void microphone_enable_audio_listening(void) {
    assert(is_initialized);
    is_running = true;

    // Start the ALSA thread to capture data
    audio_capture_start();

    // Start a background thread to listen for audio input
    pthread_create(&mic_thread, nullptr, listen_for_audio, nullptr);
}

void microphone_disable_audio_listening(void) {
    assert(is_initialized);

    audio_capture_stop();
}

const char* microphone_get_audio_input(void) {
    return audio_input_char;
}

enum keyword microphone_get_keyword_from_audio_input(void) {
    assert(is_initialized);

    if (strstr(audio_input_char, "stop")) {
        return STOP;
    } else if (strstr(audio_input_char, "next song")) {
        return NEXT;
    } else if (strstr(audio_input_char, "previous")) {
        return PREVIOUS;
    } else if (strstr(audio_input_char, "volume up")) {
        return VOLUME_UP;
    } else if (strstr(audio_input_char, "volume down")) {
        return VOLUME_DOWN;
    } else {
        return KEYWORD_NONE;
    }
}

void microphone_cleanup(void) {
    assert(is_initialized);
    is_initialized = false;
    is_running = false;

    std::free(audio_input_samples);
    audio_input_samples = nullptr;

    pthread_join(mic_thread, nullptr);
}
