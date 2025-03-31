#ifndef WHISPER_H
#define WHISPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for the opaque types
struct whisper_context;

enum whisper_sampling_strategy {
    WHISPER_SAMPLING_GREEDY,
    WHISPER_SAMPLING_BEAM_SEARCH,
};

struct whisper_full_params {
    enum whisper_sampling_strategy strategy;

    bool print_progress;
    bool print_special;
    bool print_realtime;
    bool print_timestamps;

    int n_threads;
    int max_len;
    int offset_ms;
    int duration_ms;
    bool translate;
    bool no_context;
    bool single_segment;
    bool no_timestamps;
    bool token_timestamps;

    float temperature;
    float max_initial_ts;

    const char* language;
};

// Load model from file
struct whisper_context* whisper_init_from_file(const char* path);

// Free model context
void whisper_free(struct whisper_context* ctx);

// Get default parameters
struct whisper_full_params whisper_full_default_params(enum whisper_sampling_strategy strategy);

// Run full transcription
int whisper_full(
    struct whisper_context* ctx,
    struct whisper_full_params params,
    const short* samples,
    size_t n_samples
);

// Get result text
const char* whisper_full_get_segment_text(struct whisper_context* ctx, int index);

#ifdef __cplusplus
}
#endif

#endif // WHISPER_H

