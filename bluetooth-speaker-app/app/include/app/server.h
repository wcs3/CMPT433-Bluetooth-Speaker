// Start and run cleanup for the UDP server. Server runs in a separate thread. 
// The server thread stops when a stop command is received or init_get_shutdown() is true
#ifndef _SERVER_H
#define _SERVER_H

// Starts the server thread
int server_init();

// Cleans up the resources used by the server. Does not stop the server.
void server_cleanup();

#endif
