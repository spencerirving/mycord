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

void print_error(const char* error_message) {
	// print an error to stderr
	fprintf(stderr, "Error: %s\n", error_message);
}

int process_args(int argc, char *argv[], settings_t* settings) {
	// parses the CLI arguments provided
	// returns 0 on success and -1 on failure
	settings->server.sin_family = AF_INET; // sets the address family to IPv4
	settings->server.sin_port = htons(8080); // defaults the port to 8080 in network byte order
       	inet_pton(AF_INET, "127.0.0.1", &settings->server.sin_addr); // defaults ip address to 127.0.0.1"
	bool double_ip_input = false; // used to check if a domain and an ip address are inputted
	settings->quiet = false; // defaults quiet to false

	for (int i = 1; i<argc; i++) { // loops through each cli argument
		char* arg = argv[i]; // pointer to the current argument
		if (strncmp(arg, "--help", 6) == 0 || strncmp(arg, "-h", 2) == 0) { // checks if the help flag was passed
			print_help();
			return 0;
		} else if (strncmp(arg, "--port", 6) == 0) { // checks if the port flag was passed 
			i++; // moves to the next argument which should have the port value
			if (i == argc) { // checks if no port was provided
				print_error("Missing argument after --port\n");
				return -1;
			}
			int port = atoi(argv[i]); // converts the string port value to an int
			if (port == 0 || port < 1024 || port > 65535) { 
				// checks if the ascii to integer conversion failed or if its inaccessible
				print_error("Invalid port\n"); 
				return -1;
			}
			settings->server.sin_port = htons(port); // stores the port value in network byte order
		} else if (strncmp(arg, "--ip", 4) == 0) { // checks if the ip flag was passed
			if (double_ip_input) { // checks if an domain name was already passed
				print_error("Cannot specify both IP address and domain name\n");
				return -1;
			}
			double_ip_input = true; // specifies an ip address has already been inputted
			i++; // moves to the next argument which should have the ip address
			if (i == argc) { // checks if no ip address was provided
				print_error("Missing argument after --ip\n");
				return -1; 
			}
			if (inet_pton(AF_INET, argv[i], &settings->server.sin_addr) < 1) { // sets the ip address in settings
				// checks for errors
				print_error("Invalid IP address\n");
				return -1;
			}
		} else if (strncmp(arg, "--domain", 8) == 0) { // checks if the domain flag was passed
			if (double_ip_input) { // checks if an ip address was already passed
				print_error("Cannot specify both IP address and domain name\n"); 
				return -1; 
			}
			double_ip_input = true; // specifies an domain name has already been inputted
			i++; // moves to the next argument which should have the domain name
			if (i == argc) { // checks if no ip address was provided
				print_error("Missing argument after --domain\n"); 
				return -1; 
			}
			struct hostent* host_info = gethostbyname(argv[i]); // gets the ip address and host info from a domain
			if (host_info == NULL) { 
				print_error("Could not resolve domain\n"); 
				return -1;
			}
			if (host_info->h_addrtype == AF_INET) { // checks for IPv4 address
				memcpy(&settings->server.sin_addr, host_info->h_addr_list[0], host_info->h_length);
			}
		} else if (strncmp(arg, "--quiet", 7) == 0) { // checks if the quiet flag was passed
			settings->quiet = true; // sets the quiet setting to true
		} else { 
			print_error("Invalid argument\n");
			return -1;
		}
	}
	return 0;				
}

int get_username(settings_t* settings) {
	// retrieves the username of the current user to login into mycord with
	// returns -1 on success and 0 on failure
	FILE* fp = popen("whoami", "r"); // runs the command whoami and reads from stdin
	if (fp == NULL) { // checks if popen failed
		print_error("popen failed\n");
		return -1;
	}
	if (fgets(settings->username, sizeof(settings->username), fp) == NULL) { // stores username in settings and checks for failire
		print_error("Failed to get username\n");
		return -1;
	}
	pclose(fp); // closes the file
	return 0;
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
