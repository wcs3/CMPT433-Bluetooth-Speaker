#include <hal/audio_capture.h>
#include <alsa/asoundlib.h>

#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#define SAMPLE_RATE_HZ 16000
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short))
#define ALSA_FRAMES_PER_READ 512
#define INITIAL_SIZE_AUDIO_BUFFER 1024

// The microphone is on card 2, device 0
#define DEVICE_NAME "plughw:2,0"

static snd_pcm_t* audio_capture_handle;
static pthread_t audio_capture_thread;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t buffer_ready_cond = PTHREAD_COND_INITIALIZER;
static bool is_initialzed = false;
static bool is_capturing = false;
static bool is_buffer_ready = false;

static short* audio_buffer = NULL;
static size_t buffer_size = 0; // # of samples currently in the audio buffer
static size_t buffer_capacity = 0; // The max # of samples the buffer can hold

static void* audio_capture_thread_loop(void* arg) {
    (void)arg;
    short ALSA_buffer[ALSA_FRAMES_PER_READ];

    while(is_capturing) {
        // Read the frames (containing samples) from the microphone
        snd_pcm_sframes_t frames = 
        snd_pcm_readi(audio_capture_handle, ALSA_buffer, ALSA_FRAMES_PER_READ);

        // Check and handle encountered errors
        if (frames < 0) {
            fprintf(stderr, "AudioMixer: readi() returned %li\n", frames);
			frames = snd_pcm_recover(audio_capture_handle, frames, 1);
        }

        if (frames < 0) {
            fprintf(stderr, "ERROR: Failed reading audio with snd_pcm_readi(): %li\n",
					frames);
			exit(EXIT_FAILURE);
        }

        if (frames > 0 && frames < ALSA_FRAMES_PER_READ) {
			printf("Short read (expected %d, got %ld)\n", ALSA_FRAMES_PER_READ, frames);
		}

        if (frames > 0) {
			// The case where we captured correct number of samples.
            // Copy the data into a dynamically allocated audio_buffer 
            // which grows exponentially as new data is captured through
            // the microphone.
            pthread_mutex_lock(&buffer_mutex);
            size_t new_size = buffer_size + frames;

            if (new_size > buffer_capacity) {
                if (buffer_capacity == 0) {
                    // The first iteration
                    buffer_capacity = INITIAL_SIZE_AUDIO_BUFFER;
                }

                while (buffer_capacity < new_size) {
                    // Exponentially expand the size of audio buffer to store new samples
                    buffer_capacity *= 2;
                }

                // Allocate or reallocate space for new data
                audio_buffer = realloc(audio_buffer, buffer_capacity * SAMPLE_SIZE);
                if (!audio_buffer) {
                    fprintf(stderr, "ERROR: realloc failed\n");
                    exit(EXIT_FAILURE);
                }
            }
            // Copy the new data at the correct position in audio_buffer
            memcpy(&audio_buffer[buffer_size], ALSA_buffer, frames * SAMPLE_SIZE);
            buffer_size = new_size;
            pthread_mutex_unlock(&buffer_mutex);
		}
    }

    // After the user disables the microphone (i.e. the program exits ALSA thread loop)
    // signal that the audio_buffer is ready to be translated into text
    pthread_mutex_lock(&buffer_mutex);
    is_buffer_ready = true;
    pthread_cond_signal(&buffer_ready_cond);
    pthread_mutex_unlock(&buffer_mutex);

    return NULL;
}

void audio_capture_init(void) {
    // Open the PCM input (microphone)
    int error = snd_pcm_open(&audio_capture_handle, DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0);
    if (error < 0) {
        printf("Capture open error: %s\n", snd_strerror(error));
		exit(EXIT_FAILURE);
    }

    // Configure parameters of PCM output
	error = snd_pcm_set_params(audio_capture_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        NUM_CHANNELS,
        SAMPLE_RATE_HZ,
        1,
        50000);
    if (error < 0) {
        printf("Capture open error: %s\n", snd_strerror(error));
        exit(EXIT_FAILURE);
    }

    is_initialzed = true;
}

void audio_capture_cleanup(void) {
    assert(is_initialzed);

    snd_pcm_close(audio_capture_handle);
    free(audio_buffer);
    audio_buffer = NULL;
    buffer_size = 0;
    buffer_capacity = 0;
    is_initialzed = false;
}

void audio_capture_start(void) {
    assert(is_initialzed);

    // Upon calling this function re-initiate the variables
    is_capturing = true;
    buffer_size = 0;
    buffer_capacity = 0;
    audio_buffer = NULL;
    is_buffer_ready = false;

    // Start the background thread for capturing audio
    pthread_create(&audio_capture_thread, NULL, audio_capture_thread_loop, NULL);
}

void audio_capture_stop(void) {
    assert(is_initialzed);

    // Terminate the ALSA thread loop once the user stops talking to the microphone
    is_capturing = false;
    pthread_join(audio_capture_thread, NULL);
}

void audio_capture_get_audio_buffer(short** out_buffer, size_t* out_size) {
    pthread_mutex_lock(&buffer_mutex);

    // While the user is talking to the microphone wait until all the data is captured
    // in the audio_buffer, then proceed to pass the full data captured to the VOSK thread
    // in the microphone HAL module to translate the samples into text.
    while (!is_buffer_ready) {
        pthread_cond_wait(&buffer_ready_cond, &buffer_mutex);
    }

    *out_buffer = malloc(buffer_size * SAMPLE_SIZE);
    memcpy(*out_buffer, audio_buffer, buffer_size * SAMPLE_SIZE);
    *out_size = buffer_size;
    pthread_mutex_unlock(&buffer_mutex);
}