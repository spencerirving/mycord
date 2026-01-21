# mycord Client

## Overview 

This project is a C-based client implementation for the **mycord** chat service. It was completed as **Part 1 of the Final Programming Assignment** for **CMPSC 311: Introduction to Systems Programming** at **The Pennsylvania State University**.

The goal of this project was to build a fully functional network chat client using low-level Linux/Unix system programming concepts, including sockets, threading, signals, and structured network protocols. 

This repository contains **only Part 1** of the final project, where the instructions can be found in the `INSTRUCTIONS.md` file.

## Project Description

The `client.c` program connects to a mycord server over **TCP IPv4**, logs in using the current system username, and allows users to send a recieve cxhat messages from a server.

The client implements the mycord message protocol and supports concurrent message sending and recieving through the use of POSIX threads (`pthreads`). It also properly handles server messages, system notifications, disconnects, and graceful termination via signals or EOF.

### Core Functionality
- TCP IPv4 socket connection to a mycord server
- DNS resolution when connecting via domain name
- LOGIN, LOGOUT, and MESSAGE_SEND outbound messages
- MESSAGE_RECV, SYSTEM, and DISCONNECT inbound messages
- Structured message exchange using fixed-size packed structs

### Command-Line Interface
Supports the following flags:

`--help`

`--port PORT`

`--ip IP`

`--domain DOMAIN`

`--quiet`

### Message Handling
- Receives and formats messages with timestamps
- Highlights `@mentions` of the current user in red
- Emits an audible bell (`\a`) on mention (unless `--quiet` is used)
- Displays SYSTEM messages in gray
- Displays DISCONNECT messages in red

### Concurrency
- Uses a dedicated receiving thread to handle inbound messages
- Main thread handles user input from STDIN
- Clean shutdown coordination between threads

### Input Validation & Error Handling
- Validates outgoing messages before sending
- Prevents invalid messages that would cause server disconnects
- Prints all errors to STDERR with the prefix `Error:`
- Gracefully handles:
  - SIGINT (Ctrl+C)
  - SIGTERM
  - EOF on STDIN (Ctrl+D)
 
## Message Protocol

The mycord protocol exchanges fixed-size **1,064 byte messages** consisting of:

| Field         | Size (bytes) |
|--------------|--------------|
| Message Type | 4            |
| Timestamp    | 4            |
| Username     | 32           |
| Message      | 1024         |

All numeric fields are transmitted in **network byte order**.

## File Structure 

`client.c` - Mycord client implementation

`server.py` - Teacher written test server

`messages.log` Server-generated message history

`INSTRUCTIONS.md` Project Instructions

## Compilation 

### To compile the client: 
```bash
gcc client.c -o client -pthread
```

### Local testing: 
Machine 1
```bash
python3 server.py
```
Machine 2
```bash
./client --port <server_port> --ip <machine_1_ip>
```

### Classroom server:
```bash
./client --domain mycord.devic.dev
```

