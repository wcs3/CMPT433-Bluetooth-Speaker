#define _POSIX_C_SOURCE 200809L
#include "app/server.h"
#include "app/init.h"
#include "hal/light_sensor.h"

#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

static const size_t PACKET_MAX = 1500;
static const int RECV_BUF_SIZE = 100;
static const int SEND_BUF_SIZE = 2000;
static const int NI_MAXHOST = 2000;
static const int NI_MAXSERV = 2000;
static const char* PORT = "12345";

static pthread_t server_thread;

static const char* COMMAND_HELP1 = "help\n";
static const char* COMMAND_HELP2 = "?\n";
static const char* COMMAND_COUNT = "count\n";
static const char* COMMAND_LENGTH = "length\n";
static const char* COMMAND_DIPS = "dips\n";
static const char* COMMAND_HISTORY = "history\n";
static const char* COMMAND_STOP = "stop\n";
static const char* COMMAND_REPEAT = "\n";
static const char* HELP_MSG = 
    "help / ?: Display commands\n"
    "count: Return the total number of light samples take so far\n"
    "length: Return how many samples were captured during the previous second\n"
    "dips: Return how many dips were detected during the previous second's samples\n"
    "history: Return all the data samples from the previous second\n"
    "stop: Exit program\n"
    "<enter>: Repeat previous command\n";

static int send_one_packet(int socket_fd, char* send, size_t send_amount, struct sockaddr* peer_addr, socklen_t peer_addr_len)
{
    assert(send_amount <= PACKET_MAX);
    return sendto(socket_fd, send, send_amount, 0, peer_addr, peer_addr_len );
}

// general UDP setup from https://gist.github.com/saxbophone/f770e86ceff9d488396c0c32d47b757e
void* run_server(void* arg __attribute__((unused)))
{    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    struct addrinfo *result = NULL;
    int getaddrinfo_code = getaddrinfo(NULL, PORT, &hints, &result);
    if (getaddrinfo_code != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_code));
        return (void*) 1;
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    int socket_fd = -1;
    struct addrinfo* rp = result;
    for (; rp != NULL; rp = rp->ai_next) {
        socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd == -1)
            continue;

        if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(socket_fd);
    }

    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not bind socket\n");
        return (void*) 2;
    }

    // Make the socket time out if nothing is received for one second
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "Could not set timeout on socket\n");
        close(socket_fd);
        return (void*) 3;
    }

    freeaddrinfo(result);           /* No longer needed */

    bool stop_command_received = false;
    char* recv_buf = malloc(RECV_BUF_SIZE * sizeof(char));
    char* send_buf = malloc(SEND_BUF_SIZE * sizeof(char));
    char* prev_cmd = malloc(RECV_BUF_SIZE * sizeof(char));
    prev_cmd[0] = 'x';
    prev_cmd[1] = '\0';

    while(!stop_command_received && !init_get_shutdown()) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
        
        ssize_t bytes_read = recvfrom(socket_fd, recv_buf, RECV_BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
        // failed request or timeout
        if(bytes_read < 0) {
            continue;
        }

        char host[NI_MAXHOST], service[NI_MAXSERV];
        int getnameinfo_code = getnameinfo(
            (struct sockaddr *) &peer_addr,
            peer_addr_len,
            host,
            NI_MAXHOST,
            service,
            NI_MAXSERV,
            NI_NUMERICSERV
        );

        if (getnameinfo_code == 0) {
            // printf("Received %zd bytes from %s:%s\n", bytes_read, host, service);
        } else {
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(getnameinfo_code));
        }

        // tokenize
        char *save_ptr;
        const char delim[] = " ";
        recv_buf[bytes_read] = '\0';
        char* command_str = strtok_r(recv_buf, delim, &save_ptr);
        // printf("%s\n", command_str);

        // handle repeat command
        if(strcmp(command_str, COMMAND_REPEAT) == 0) {
            strncpy(recv_buf, prev_cmd, RECV_BUF_SIZE - 1);
            send_buf[RECV_BUF_SIZE - 1] = '\0';
        } else {
            strncpy(prev_cmd, recv_buf, RECV_BUF_SIZE - 1);
            prev_cmd[RECV_BUF_SIZE - 1] = '\0';
        }
        
        // build return message based on command
        send_buf[0] = '\0';
        if(strcmp(command_str, COMMAND_HELP1) == 0|| strcmp(command_str, COMMAND_HELP2) == 0) {
            strncpy(send_buf, HELP_MSG, SEND_BUF_SIZE - 1);
            send_buf[SEND_BUF_SIZE - 1] = '\0';

            send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
        } else if(strcmp(command_str, COMMAND_COUNT) == 0) {
            snprintf(send_buf, SEND_BUF_SIZE, "samples taken since program start: %ld\n", light_sensor_samples_taken());

            send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
        } else if(strcmp(command_str, COMMAND_LENGTH) == 0) {
            struct light_sensor_statistics stats;
            light_sensor_history_1sec(&stats);
            snprintf(send_buf, SEND_BUF_SIZE, "samples taken in last second: %d\n", stats.size);
            light_sensor_statistics_free(&stats);

            send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
        } else if(strcmp(command_str, COMMAND_DIPS) == 0) {
            struct light_sensor_statistics stats;
            light_sensor_history_1sec(&stats);
            snprintf(send_buf, SEND_BUF_SIZE, "light dips detected: %d\n", light_sensor_count_dips(&stats));
            light_sensor_statistics_free(&stats);

            send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
        } else if(strcmp(command_str, COMMAND_HISTORY) == 0) {
            struct light_sensor_statistics stats;
            light_sensor_history_1sec(&stats);
            // each number takes at most 5 digits and one comma, one newline char per line
            int max_lines_per_packet = PACKET_MAX / ((LIGHT_SENSOR_MAX_VOLTAGE_DIGITS + 5) * 10 + 1);
            
            // send packets with no more than max_lines_per_packet * 10 numbers each
            int str_end_offset = 0;
            bool numbers_unsent = true;
            for(int i = 0; i < stats.size; i++) {
                numbers_unsent = true;
                // 10th line or last number
                if((i + 1) % 10 == 0 || i == stats.size - 1) {
                    snprintf(send_buf + str_end_offset, SEND_BUF_SIZE - str_end_offset, "%.3f,\n", stats.readings[i]);
                } else {    
                    snprintf(send_buf + str_end_offset, SEND_BUF_SIZE - str_end_offset, "%.3f,", stats.readings[i]);
                }
                str_end_offset += strlen(send_buf + str_end_offset);
                
                if((i + 1) % (max_lines_per_packet * 10) == 0) {
                    send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
                    str_end_offset = 0;
                    numbers_unsent = false;
                }
            }
            if(numbers_unsent) {
                send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
            }
            
            light_sensor_statistics_free(&stats);
        } else if(strcmp(command_str, COMMAND_STOP) == 0) {
            stop_command_received = true;
            snprintf(send_buf, SEND_BUF_SIZE, "Program stopped\n");
            send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
        } else {
            snprintf(send_buf, SEND_BUF_SIZE, "Command not recognized. Type help\n");
            send_one_packet(socket_fd, send_buf, strlen(send_buf), (struct sockaddr *) &peer_addr, peer_addr_len);
        }
    }

    free(recv_buf);
    free(send_buf);
    free(prev_cmd);
    close(socket_fd);

    init_set_shutdown();
    init_signal_done();
    return NULL;
}

int server_init()
{
    int thread_code = pthread_create(&server_thread, NULL, run_server, NULL);
    if(thread_code) {
        fprintf(stderr, "failed to create server thread %d\n", thread_code);
        return 1;
    }
    return 0;
}


void server_cleanup()
{
    void* result;
    int thread_code = pthread_join(server_thread, &result);
    if(thread_code) {
        fprintf(stderr, "failed to join server thread %d\n", thread_code);
    }
    printf("server code %ld\n", (long) result);
}