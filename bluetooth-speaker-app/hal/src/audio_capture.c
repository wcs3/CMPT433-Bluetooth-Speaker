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
#define BUFFER_DURATION_SEC 2

// The microphone is on card 2, device 0
#define DEVICE_NAME "plughw:2,0"

// the number of frames captured from the microphone
// by ALSA at every iteration of the thread (holds 2s of data)
#define ALSA_BUFFER_SIZE (SAMPLE_RATE_HZ * BUFFER_DURATION_SEC)
#define AUDIO_BUFFER_SIZE (SAMPLE_RATE_HZ * BUFFER_DURATION_SEC)

static snd_pcm_t* audio_capture_handle;
static pthread_t audio_capture_thread;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool is_initialzed = false;
static bool is_capturing = false;

// Buffer storing captured data
static short audio_buffer[AUDIO_BUFFER_SIZE];

static void* audio_capture_thread_loop(void* arg) {
    (void)arg;
    short ALSA_buffer[ALSA_BUFFER_SIZE];

    while(is_capturing) {
        // Read the frames (containing samples) from the microphone
        snd_pcm_sframes_t frames = 
        snd_pcm_readi(audio_capture_handle, ALSA_buffer, ALSA_BUFFER_SIZE);

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

        if (frames > 0 && frames < ALSA_BUFFER_SIZE) {
			printf("Short read (expected %li, got %li)\n",
					ALSA_BUFFER_SIZE, frames);
		}

        else {
			// The case where we read correct number of frames
			// Copy the result into the bigger buffer
			pthread_mutex_lock(&buffer_mutex);
            memcpy(&audio_buffer, ALSA_buffer, frames * SAMPLE_SIZE);
            pthread_mutex_unlock(&buffer_mutex);
		}
    }

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
    is_initialzed = false;
}

void audio_capture_start(void) {
    assert(is_initialzed);
    is_capturing = true;

    // Start the background thread for capturing audio
    pthread_create(&audio_capture_thread, NULL, audio_capture_thread_loop, NULL);
}

void audio_capture_stop(void) {
    assert(is_initialzed);
    is_capturing = false;
    pthread_join(audio_capture_thread, NULL);
}

void audio_capture_get_audio_buffer(short* out_buffer) {
    pthread_mutex_lock(&buffer_mutex);
    memcpy(out_buffer, &audio_buffer, AUDIO_BUFFER_SIZE * SAMPLE_SIZE);
    pthread_mutex_unlock(&buffer_mutex);
}