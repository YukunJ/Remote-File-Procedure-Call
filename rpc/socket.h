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
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* the local address by default */
#define DEFAULT_ADDRESS "127.0.0.1"

/* the port by default */
#define DEFAULT_PORT "15440"

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

#endif  // SOCKET_H
