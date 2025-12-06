#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

// typedef enum MessageType { ... } message_type_t;
// typedef struct __attribute__((packed)) Message { ... } message_t;
typedef struct Settings {
    struct sockaddr_in server;
    bool quiet;
    int socket_fd;
    bool running;
    char username[32];
} settings_t;

static char* COLOR_RED = "\033[31m";
static char* COLOR_GRAY = "\033[90m";
static char* COLOR_RESET = "\033[0m";
static settings_t settings = {0};


void print_help() { 
	// this function prints the help function out to stdout when called by the main function
	fprintf(stdout, "usage: ./client [-h] [--port PORT] [--ip IP] [--domain DOMAIN] [--quiet]\n\n"

		"mycord client\n\n"
		"options:\n"
		"  --help                show this help message and exit\n"
		"  --port PORT           port to connect to (default: 8080)\n"
		"  --ip IP               IP to connect to (default: \"127.0.0.1\")\n"
		"  --domain DOMAIN       Domain name to connect to (if domain is specified, IP must not be)\n"
		"  --quiet               do not perform alerts or mention highlighting\n\n"
		"examples:\n"
		"  ./client --help (prints the above message)\n"
		"  ./client --port 1738 (connects to a mycord server at 127.0.0.1:1738)\n"
		"  ./client --domain example.com (connects to a mycord server at example.com:8080)\n"
  );
}

int process_args(int argc, char *argv[]) {
    return -1;
}

int get_username() {
    return -1;
}

void handle_signal(int signal) {
    return;
}

ssize_t perform_full_read(void *buf, size_t n) {
    return -1;
}

void* receive_messages_thread(void* arg) {
    // while some condition(s) are true
        // read message from the server (ensure no short reads)
        // check the message type
            // for message types, print the message and do highlight parsing (if not quiet)
            // for system types, print the message in gray with username SYSTEM
            // for disconnect types, print the reason in red with username DISCONNECT and exit
            // for anything else, print an error
}

int main(int argc, char *argv[]) {
    // setup sigactions (ill-advised to use signal for this project, use sigaction with default (0) flags instead)

    // parse arguments

    // get username

    // create socket

    // connect to server

    // create and send login message

    // create and start receive messages thread

    // while some condition(s) are true
        // read a line from STDIN
        // do some error checking (handle EOF, EINTR, etc.)
        // send message to the server
    
    // wait for the thread / clean up

    // cleanup and return
    print_help();
}
