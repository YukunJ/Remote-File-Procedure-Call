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

/* the waiting queue allowed for server */
#define WAIT_LOG 64

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
 * @brief robust receive data from the socket
 * @param fd the socket to receive from
 * @param buf_start the beginning of a buffer to write data into
 * @param buf_size the maximum capacity of the buffer
 * @param exit if the sender exits, exit is set to true
 * @return how many bytes are actually read and store in 'buf_start'
 */
ssize_t robust_read(int fd, char* buf_start, size_t buf_size, bool* exit);

#endif  // SOCKET_H
