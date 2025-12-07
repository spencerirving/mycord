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
enum MessageType { 
	LOGIN = 0,
	LOGOUT = 1,
	MESSAGE_SEND = 2,
	MESSAGE_RECV = 10,
	DISCONNECT = 12,
	SYSTEM = 13
} message_type_t;

// typedef struct __attribute__((packed)) Message { ... } message_t;
typedef struct __attribute__((packed)) Message { 
	unsigned int message_type;
	unsigned int timestamp;
	char username[32];
	char message[1024];
} message_t;


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
	bool double_ip_input = false; // used to check if a domain and an ip address are inputted

	for (int i = 1; i<argc; i++) { // loops through each cli argument
		char* arg = argv[i]; // pointer to the current argument
		if (strncmp(arg, "--help", 6) == 0 || strncmp(arg, "-h", 2) == 0) { // checks if the help flag was passed
			print_help();
			return 0;
		} else if (strncmp(arg, "--port", 6) == 0) { // checks if the port flag was passed 
			i++; // moves to the next argument which should have the port value
			if (i == argc) { // checks if no port was provided
				print_error("Missing argument after --port");
				return -1;
			}
			int port = atoi(argv[i]); // converts the string port value to an int
			if (port == 0 || port < 1024 || port > 65535) { 
				// checks if the ascii to integer conversion failed or if its inaccessible
				print_error("Invalid port"); 
				return -1;
			}
			settings->server.sin_port = htons(port); // stores the port value in network byte order
		} else if (strncmp(arg, "--ip", 4) == 0) { // checks if the ip flag was passed
			if (double_ip_input) { // checks if an domain name was already passed
				print_error("Cannot specify both IP address and domain name");
				return -1;
			}
			double_ip_input = true; // specifies an ip address has already been inputted
			i++; // moves to the next argument which should have the ip address
			if (i == argc) { // checks if no ip address was provided
				print_error("Missing argument after --ip");
				return -1; 
			}
			if (inet_pton(AF_INET, argv[i], &settings->server.sin_addr) < 1) { // sets the ip address in settings
				// checks for errors
				print_error("Invalid IP address");
				return -1;
			}
		} else if (strncmp(arg, "--domain", 8) == 0) { // checks if the domain flag was passed
			if (double_ip_input) { // checks if an ip address was already passed
				print_error("Cannot specify both IP address and domain name"); 
				return -1; 
			}
			double_ip_input = true; // specifies an domain name has already been inputted
			i++; // moves to the next argument which should have the domain name
			if (i == argc) { // checks if no ip address was provided
				print_error("Missing argument after --domain"); 
				return -1; 
			}
			struct hostent* host_info = gethostbyname(argv[i]); // gets the ip address and host info from a domain
			if (host_info == NULL) { 
				print_error("Could not resolve domain"); 
				return -1;
			}
			if (host_info->h_addrtype == AF_INET) { // checks for IPv4 address
				memcpy(&settings->server.sin_addr, host_info->h_addr_list[0], host_info->h_length);
			}
		} else if (strncmp(arg, "--quiet", 7) == 0) { // checks if the quiet flag was passed
			settings->quiet = true; // sets the quiet setting to true
		} else { 
			print_error("Invalid argument");
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
		print_error("popen failed");
		return -1;
	}
	if (fgets(settings->username, sizeof(settings->username), fp) == NULL) { // stores username in settings and checks for failire
		print_error("Failed to get username");
		return -1;
	}
	pclose(fp); // closes the file
	// remove the new line 
	size_t len = strnlen(settings->username, sizeof(settings->username));
	if (len > 0 && settings->username[len-1] == '\n') { // checks for a new line character 
		settings->username[len-1] = '\0'; // replaces it with null terminator
	}
	return 0;
}

void handle_signal(int signal) {
	// called to handle signals
	if (signal == SIGINT || signal == SIGTERM) { 
		settings.running = false;
	}
	return;
}

ssize_t perform_full_write(const void* buf, size_t n, int socket_fd) { 
	// performs a full write to the server
	size_t total_written = 0;

	while (total_written < n) { 
		ssize_t bytes_written = write(socket_fd, (const char*)buf+total_written, n-total_written); // writes to the socket
		if (bytes_written == -1) { // checks if the write failed
			if (errno == EINTR) { // signal interrupt 
				continue;
			}
			print_error(strerror(errno)); 
			return -1; 
		}
		if (bytes_written == 0) { // cant write anymore
			return total_written; 
		}
		total_written += bytes_written; // increases total wrote
	}
	return total_written;
}

ssize_t perform_full_read(void* buf, size_t n, int socket_fd) {
	// ensures a full read from the server
	size_t total_read = 0; // stores total elements read 

	while (total_read < n) { 
		ssize_t bytes_read = read(socket_fd, (char*)buf+total_read, n-total_read); // read from socket
		if (bytes_read == -1) { // checks if read failed
			if (errno == EINTR) { // signal interrupt
				continue;
			}
			print_error(strerror(errno));
			return -1;
		} 
		if (bytes_read == 0) { // not reading anymore
			return total_read;
		}
		total_read+=bytes_read; // increases total read
	}
	return total_read;
}

void* receive_messages_thread(void* arg) {
	// worker thread to receive messages from the server
	// while some condition(s) are true
	settings_t* settings = (settings_t*) arg; // casts the settings from the argument
	if (settings == NULL) { // checks if arg is null
	       print_error("Failed to pass settings to worker");
	       pthread_exit((void*)-1);
    	}	       

	while (settings->running) { // does work as long as the client is connected to the server
		message_t message; // new message struct

		// read message from the server (ensure no short reads)
		ssize_t size = perform_full_read(&message, sizeof(message_t), settings->socket_fd); 

		if (size != sizeof(message_t)) { // checks for an incomplete read
			if (!settings->running) { // checks to see if the server is shut down
				break;
			}
			print_error("Failed to read message from server");
			pthread_exit((void*)-1);
		}
		
		// convert from network to host byte order
		message.message_type = ntohl(message.message_type); 
		message.timestamp = ntohl(message.timestamp);

		// check the message type
		if (message.message_type == MESSAGE_RECV) { // checks if the message from the server is MESSAGE_RECV type
			// convert the timestamp to a printable time
			time_t t = (time_t) message.timestamp; 
			struct tm *time = localtime(&t);
			char time_str[64]; 
			strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time); 
			
			if (settings->quiet) { // checks if the quiet parameter was specified
				printf("[%s] %s: %s\n", time_str, message.username, message.message);
			} else {
                                printf("[%s] %s: ", time_str, message.username);
				char compare[34] = "@"; // used to check for the mention
				strncat(compare, settings->username, 33); // concats "@" and the persons username together
				size_t length = strnlen(message.message, 1024); // length of the message
				size_t username_length = strnlen(compare, 33); // length of the username plus the @ symbol
				bool mentioned = false; 

				for (size_t i = 0; i<length; ) { 
					if (i+username_length <= length && strncmp(&(message.message[i]), compare, username_length) == 0) { 
						// checks if current position plus username_length is still in bounds
						// compares the current string and the username to see if they match
						if (!mentioned) { // checks if there hasnt been a mention yet
							printf("\a"); // sends the bell character
						}
						mentioned = true; // sets mentioned to true so the bell only gets sent once
						printf("%s%s%s", COLOR_RED, compare, COLOR_RESET); // prints the highlighted username to stdout
						i+=username_length; // increments i by username length
					} else { 
						printf("%c", message.message[i]); // prints the current character to stdout
						i++;
					}
				}
				printf("\n"); // adds a new line
			}
		} else if (message.message_type == DISCONNECT) { // checks if the message from the server is DISCONNECT type
			printf("%s[DISCONNECT] %s%s\n", COLOR_RED, message.message, COLOR_RESET); // prints the disconnect message to stdout
		} else if (message.message_type == SYSTEM) { // checks if the message from the server is SYSTEM type
			printf("%s[SYSTEM] %s%s\n", COLOR_GRAY, message.message, COLOR_RESET); // prints the disconnect message to stdout
		} else { // invalid inbound message
			print_error("Invalid inbound message from server"); 
			pthread_exit((void*)-1);
		}
	}
	return NULL; 
}

int main(int argc, char *argv[]) {
	// setup sigactions (ill-advised to use signal for this project, use sigaction with default (0) flags instead)
	struct sigaction sa = {0}; 
	sa.sa_handler = handle_signal;
	if (sigaction(SIGINT, &sa, NULL) == -1) { // sets up SIGINT handler and checks for error
		print_error("Failure to setup SIGINT signal handler");
		return -1;
	}
	if (sigaction(SIGTERM, &sa, NULL) == -1) { // sets up SIGTERM handler and checks for error
		print_error("Failure to setup SIGTERM signal handler"); 
		return -1;
	}
	
	// set up default settings
	settings.server.sin_family = AF_INET; // defaults address family to IPv4
	settings.server.sin_port = htons(8080); // defaults port to 8080 in network byte order
	settings.quiet = false; // defaults quiet to false
	settings.socket_fd = -1; // defaults the socket to -1
	settings.running = false; // defaults running to false
	inet_pton(AF_INET, "127.0.0.1", &(settings.server.sin_addr)); // defaults ip address to 127.0.0.1

	// parse arguments
	if (process_args(argc, argv, &settings) == -1) { // checks if processing arguments failed  
		close(settings.socket_fd);
		return -1;
	}

	// get username
	if (get_username(&settings) == -1) { // checks if get_username failed
                close(settings.socket_fd);
		return -1;
	}

	// create socket
	settings.socket_fd = socket(AF_INET, SOCK_STREAM, 0); // creates a IPv4 socket using default TCP/Stream Protocol
	if (settings.socket_fd == -1) { // checks if creating the socket failed
		print_error("Failure to create the socket");
                close(settings.socket_fd);
		return -1;
	}	

	// connect to server
	if (connect(settings.socket_fd, (const struct sockaddr*)&(settings.server), sizeof(settings.server)) == -1) {
		// checks if connecting to the server failed
		print_error(strerror(errno));
	}
	settings.running = true; // sets running to true after connecting to the server

	// creates the login message
	message_t login_message = {
		.message_type = LOGIN, // Type 0 LOGIN [OUTBOUND]
	};
	// sets the username of the message to current username from settings 
	if (strncpy(login_message.username, settings.username, 32) == NULL) { // checks if strncpy failed
		print_error("Failed to copy username from setings"); 
		close(settings.socket_fd);
		return -1; 
	}
	login_message.message_type = htons(login_message.message_type); // converts the message type to network byte order

	// sends the login message
	if (write(settings.socket_fd, &login_message, sizeof(login_message)) <= 0) { // checks if the message failed to send
		print_error("Failed to write to server");
		print_error(strerror(errno)); 
		close(settings.socket_fd); // closes the socket
		return -1;
	}

	// create and start receive messages thread
	void* status; // stores the status of the exited thread
	pthread_t receive_messages; // declare a new thread

	// creates a worker thread
	if (pthread_create(&receive_messages, NULL, receive_messages_thread, &settings) != 0) { // checks for failure
		print_error("Failed to create recieve messages thread");
		close(settings.socket_fd);
		return -1;
	}
	
	char input_buffer[1024];
	while (settings.running) {
		// reads a line from stdin
		if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) { // checks for failure or EOF
			break;
		}
		
		// get rid of newline character
		size_t len = strnlen(input_buffer, sizeof(input_buffer));
		if (len > 0 && input_buffer[len-1] == '\n') { // checks for new line character
			input_buffer[len-1] = '\0'; // replaces new line with null terminator
			len--; // updates the length 
		}

		if (input_buffer[0] == '\0') { // checks for empty messages
			continue; // skips the empty message
		}

		if (len < 1 || len > 1023) { // checks for invalid size
			print_error("Message must be between 1 amnd 1023 characters");
			continue; // skips invalid message 
		}

		bool valid = true;
		for (size_t i = 0; i<len; i++) { // loops through the inputted string
			if (input_buffer[i] == '\n') { // checks for a new line character in the middle of the input
				print_error("Message cannot contain newlines");
				valid = false;
				break; // skips checking the rest of the message
			}
			if (!isprint((unsigned char)input_buffer[i])) { // checks if the character is printable
				print_error("Message must contain printable characters only");
				valid = false;
				break; // skips checking the rest of the message
			}
		}
		if (!valid) { // checks if the message wasn't valid
			continue; // skips the invalid message 
		}

		message_t outbound_message = {0}; // creates a new message
		outbound_message.message_type = htonl(MESSAGE_SEND); // sets message type to MESSAGE_SEND in network byte order
		strncpy(outbound_message.message, input_buffer, 1023); // copies the input buffer over
		outbound_message.message[1023] = '\0'; // makes sure its null terminated

		// sends the message to server
		if (perform_full_write(&outbound_message, sizeof(outbound_message), settings.socket_fd) != sizeof(outbound_message)) {
		       	// checks if the full write failed
		      	print_error("Failed to write to server");
			break;	       
		}
	}	 
  	settings.running = false; // EOF or error
	
	message_t logout_message = {0}; // creates a logout message
	logout_message.message_type = htonl(LOGOUT); // sets message type to LOGOT in network byte order
	if (perform_full_write(&logout_message, sizeof(logout_message), settings.socket_fd) != sizeof(logout_message)) { 
		// checks if full write failed
		print_error("Failed to send logout message to server");
		close(settings.socket_fd);
		return -1; 
	}

	// while some condition(s) are true
		// read a line from STDIN
		// do some error checking (handle EOF, EINTR, etc.)
		// send message to the server
	// wait for the thread / clean up

	pthread_join(receive_messages, &status); // waits for the thread to exit

	if (status != NULL) { // checks for failure in worker thread
		close(settings.socket_fd);
		return -1;
	}

	// cleanup and return
	close(settings.socket_fd); // closes the socket
	
}
