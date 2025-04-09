#include "hal/microphone.h"
#include "hal/audio_capture.h"
#include "../vosk/vosk_api.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <json-c/json.h>

#define MAX_INPUT 2048
#define SAMPLE_RATE 16000
#define SAMPLE_SIZE (sizeof(short))

static char audio_input_char[MAX_INPUT]; // Stores the raw input from the microphone
static short* audio_input_samples = NULL;
static size_t num_samples = 0;
static bool is_initialized = false;
static bool is_running = false;
static bool is_transcription_ready = false;
static pthread_t mic_thread;
static VoskModel* model = NULL;
static VoskRecognizer* recognizer = NULL;


static void* listen_for_audio(void* arg) {
    (void)arg;

    while (is_running) {
        // Capture the audio samples from ALSA
        audio_capture_get_audio_buffer(&audio_input_samples, &num_samples);

        // speech-to-text transcription
        if (num_samples > 0 && recognizer != NULL) {
            int accepted = vosk_recognizer_accept_waveform_s(recognizer, audio_input_samples, (int)num_samples);
            const char* result = NULL;

            if (accepted) {
                // The case where VOSK detected a phrase
                result = vosk_recognizer_result(recognizer);
            } else {
                result = vosk_recognizer_partial_result(recognizer);
            }

            // Utilized CHATGPT for this section
            // Extract the text portion from the json
            struct json_object* parsed = json_tokener_parse(result);
            if (!parsed) {
                fprintf(stderr, "VOSK: Failed to parse JSON result: %s\n", result);
                continue;
            }

            struct json_object* text;
            struct json_object* partial;

            if (json_object_object_get_ex(parsed, "text", &text)) {
                strncpy(audio_input_char, json_object_get_string(text), MAX_INPUT - 1);
                audio_input_char[MAX_INPUT - 1] = '\0';
                is_transcription_ready = true;
            }
            
            else if (json_object_object_get_ex(parsed, "partial", &partial)) {
                strncpy(audio_input_char, json_object_get_string(partial), MAX_INPUT - 1);
                audio_input_char[MAX_INPUT - 1] = '\0';
                is_transcription_ready = true;
            }

            json_object_put(parsed);
        }
    }

    return NULL;
}

void microphone_init(void) {
    is_initialized = true;

    vosk_set_log_level(0);

    // Aquire the VOSK model
    model = vosk_model_new("/mnt/remote/433-project/vosk-model-small-en-us-0.15");
    if (!model) {
        fprintf(stderr, "Failed to load VOSK model\n");
        exit(EXIT_FAILURE);
    }

    // Aquire the recognizer
    recognizer = vosk_recognizer_new(model, SAMPLE_RATE);
    if (!recognizer) {
        fprintf(stderr, "Failed to load recognizer\n");
        exit(EXIT_FAILURE);
    }

}

void microphone_enable_audio_listening(void) {
    assert(is_initialized);
    microphone_reset_audio_input();
    vosk_recognizer_reset(recognizer);
    is_running = true;

    // Start the ALSA thread to capture data
    audio_capture_start();

    // Start a background thread to listen for audio input
    pthread_create(&mic_thread, NULL, listen_for_audio, NULL);
}

void microphone_disable_audio_listening(void) {
    assert(is_initialized);
    is_running = false;

    audio_capture_stop();
    pthread_join(mic_thread, NULL);
}

const char* microphone_get_audio_input(void) {
    if (is_transcription_ready) {
        is_transcription_ready = false;
        return audio_input_char;
    } else {
        return "";
    }
}

enum keyword microphone_get_keyword_from_audio_input(void) {
    assert(is_initialized);

    if (strstr(audio_input_char, "stop")) {
        return STOP;
    } else if (strstr(audio_input_char, "play")) {
        return PLAY;
    } else if (strstr(audio_input_char, "next")) {
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

void microphone_reset_audio_input(void) {
    memset(audio_input_char, 0, MAX_INPUT);
    is_transcription_ready = false;
}

void microphone_cleanup(void) {
    assert(is_initialized);
    is_initialized = false;

    if (recognizer) {
        vosk_recognizer_free(recognizer);
        recognizer = NULL;
    }

    if (model) {
        vosk_model_free(model);
        model = NULL;
    }

    free(audio_input_samples);
    audio_input_samples = NULL;
}

