// Functions for setting up, doing cleanup, signalling to end the program
// Each application thread should call init_signal_done()
#ifndef _APP_INIT_H
#define _APP_INIT_H

#include <stdbool.h>
// Num_confirms is the amount of times init_signal_done() has to be called before the cleanup code in init_end is run
int init_start(int num_confirms);
// Sets intention to shut down 
void init_set_shutdown();
// Get whether init_set_shutdown() has been called
bool init_get_shutdown();
// Signal that the calling thread is done. Returns when init_signal_done() has been called num_confirms times.
void init_signal_done();
// Finishes after init_signal_done() is called num_confirms times
void init_end(void);

#endif