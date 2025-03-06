// Creates UI thread responsible for the LCD and terminal output. Sets up listener for rotary encoder. UI thread stops when init_get_shutdown() is true.
#ifndef _USER_INTERFACE_H
#define _USER_INTERFACE_H

#include <stdbool.h>

// Sets up listener for rotary encoder. Starts UI thread
int user_interface_init();

// Cleans up the resources used by the UI. Does not stop the UI thread.
void user_interface_cleanup();

#endif

