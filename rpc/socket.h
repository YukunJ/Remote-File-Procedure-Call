/**
 * @file socket.h
 * @author Yukun J
 * @init_date Jan 17 2023
 * @reference1 <Beej's Guide to Network Programming>
 * @reference2 CMU 15213 course's proxy-lab starter code
 * @reference3 tcp-sample/{client, server}.c
 *
 * This file includes a few network socket utility functions to be used
 * in this project - remote file procedure call
 */

#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* the local address by default */
#define DEFAULT_ADDRESS "127.0.0.1"

/* the port by default */
#define DEFAULT_PORT "20080"

/* storage buffer to store all bytes received from the socket, 1MB by default */
#define STORAGE_SIZE (1024 * 1024)

/* the waiting queue allowed for server */
#define WAIT_LOG 64

/* the local buffer size for each read from socket */
#define BUF_SIZE 1024

/* the header for message length, should be the first line of the message */
#define HEADER_MSG_LEN "Message-Length"

/* colon */
#define COLON ":"

/* the carriage return plus new line used in HTTP */
#define CRLF "\r\n"

/* the spliter between the message header and message content in communcation */
#define HEADER_SPLIT "\r\n\r\n"

/* the maximum number of bytes a header could span */
#define HEADER_MAX_LEN 128

/**
 * @brief build a client TCP socket connected to the server
 * the server ip and port is extracted out from env variable, or default if not
 * specified
 * @return the client socket descriptor, or -1 if any error
 */
int build_client();

/**
 * @brief build a listening server TCP socket
 * the port is extracted out from env variable, or default if not specified
 * @return the listening socket descriptor, or -1 if any error
 */
int build_server();

/**
 * @brief accept incoming client TCP connection
 * @param listen_fd the built listening socket
 * @return a newly-accepted client socket, or -1 if any error
 */
int accept_client(int listen_fd);

/**
 * @brief robust write through the socket
 * @param fd the socket to send to
 * @param buf_start the beginning of a buffer of data to send
 * @param to_write how many bytes to be sent
 * @return how many bytes are actually sent, should be equal 'to_write'
 */
ssize_t robust_write(int fd, char* buf_start, size_t to_write);

/**
 * @brief greedily receive all data from the socket
 * @param fd the socket to receive from, must be non-blocking
 * @param buf_start the beginning of a buffer to write data into
 * @param buf_size the maximum capacity of the buffer
 * @param exit if the sender exits, exit is set to true
 * @return how many bytes are actually read and store in 'buf_start'
 */
ssize_t greedy_read(int fd, char* buf_start, size_t buf_size, bool* exit);

/**
 * @brief send a message to the other side of connection using my protocol
 * @param fd the socket to send to
 * @param buf_start the beginning of a buffer of data to send
 * @param to_write how many bytes to be sent
 */
void send_message(int fd, char* buf_start, size_t to_write);

/**
 * @brief receive a complete message from the other side using my protocol
 * @param buf a temporary buffer that store all the bytes received from a socket
 * @param buf_content_ptr a pointer to how many bytes are available in the
 * buffer
 * @return a pointer to a dynamically allocated array of message, to be freed by
 * user. and this func will prune the buffer and its size by removing the
 * message parsed or if no message available for now, return NULL ptr
 */
char* parse_message(char* buf, size_t* buf_size_ptr);

#endif  // SOCKET_H
