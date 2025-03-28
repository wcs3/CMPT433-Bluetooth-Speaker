#ifndef _AUDIO_CAPTURE_H_
#define _AUDIO_CAPTURE_H_

#define AUDIO_BUFFER_SIZE 256

/*
Explanation of each function provided by this module:
1. audio_capture_init:
    Initializes the ALSA input stream which is the microphone

2. audio_capture_cleanup:
    Frees any allocated resources

3. audio_capture_start:
    Starts a background thread that captures the audio using the
    microphone and fills a buffer with the captures data.
    The captured data is in the format:
        - Signed 16-bit little-endian PCM samples
        - Mono audio
        - 16000 Hz
    NOTE: This format is compatible with the VOSK libarary used in microphone.c
    to translate this data into text.

4. audio_capture_stop
    Stops the background thread

5. audio_capture_get_audio_buffer
    Copies the most recent audio buffer into the provided buffer &
    NOTE: The number of written samples is equal to AUDIO_BUFFER_SIZE
*/

void audio_capture_init(void);
void audio_capture_cleanup(void);
void audio_capture_start(void);
void audio_capture_stop(void);
void audio_capture_get_audio_buffer(short* out_buffer);

#endif