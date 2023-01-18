/**
 * @file socket.c
 * @author Yukun J
 * @init_date Jan 17 2023
 *
 * This is the implementation file for socket.h. See details there.
 */

#include "socket.h"

/**
 * @brief build a client TCP socket connected to the server
 * the server ip and port is extracted out from env variable, or default if not
 * specified
 * @return the client socket descriptor, or -1 if any error
 */
int build_client() {
  int fd;
  char *server_ip;
  char *server_port;
  struct sockaddr_in addr;
  unsigned short port;
  server_ip = getenv("server15440");
  if (!server_ip) {
    server_ip = DEFAULT_ADDRESS;
  }

  server_port = getenv("serverport15440");
  if (!server_port) {
    server_port = DEFAULT_PORT;
  }

  port = (unsigned short)atoi(server_port);
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    err(EXIT_FAILURE, "client socket creation failure");
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(server_ip);
  addr.sin_port = htons(port);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
    err(EXIT_FAILURE, "client fd connect failure");
  }

  return fd;
}

/**
 * @brief build a listening server TCP socket
 * the port is extracted out from env variable, or default if not specified
 * @return the listening socket descriptor, or -1 if any error
 */
int build_server() {
  int listen_fd;
  char *server_port;
  struct sockaddr_in addr;
  unsigned short port;

  server_port = getenv("serverport15440");
  if (!server_port) {
    server_port = DEFAULT_PORT;
  }

  port = (unsigned short)atoi(server_port);
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    err(EXIT_FAILURE, "listen socket creation failure");
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
    err(EXIT_FAILURE, "listen socket bind failure");
  }

  if (listen(listen_fd, WAIT_LOG) < 0) {
    err(EXIT_FAILURE, "listen socket listen failure");
  }

  return listen_fd;
}

/**
 * @brief accept incoming client TCP connection
 * @param listen_fd the built listening socket
 * @return a newly-accepted client socket, or -1 if any error
 */
int accept_client(int listen_fd) {
  socklen_t sa_size;
  int client_fd;
  struct sockaddr_in client_addr;
  sa_size = sizeof(struct sockaddr_in);
  client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &sa_size);
  if (client_fd < 0) {
    err(EXIT_FAILURE, "listen socket accept failure");
  }

  return client_fd;
}

/**
 * @brief robust write through the socket
 * @param fd the socket to send to
 * @param buf_start the beginning of a buffer of data to send
 * @param to_write how many bytes to be sent
 * @return how many bytes are actually sent, should be equal 'to_write'
 */
ssize_t robust_write(int fd, char *buf_start, size_t to_write) {
  ssize_t curr_write = 0;
  ssize_t write;
  while (curr_write < to_write) {
    if ((write = send(fd, buf_start + curr_write, to_write - curr_write, 0)) <
        0) {
      if (errno != EINTR || errno != EAGAIN) {
        // problem happens
        return curr_write;
      }
      write = 0;
    }
    curr_write += write;
  }
  return curr_write;
}

/**
 * @brief robust receive data from the socket
 * @param fd the socket to receive from
 * @param buf_start the beginning of a buffer to write data into
 * @param buf_size the maximum capacity of the buffer
 * @param exit if the sender exits, exit is set to true
 * @return how many bytes are actually read and store in 'buf_start'
 */
ssize_t robust_read(int fd, char *buf_start, size_t buf_size, bool *exit) {
  ssize_t curr_read = 0;
  ssize_t read;
  while (curr_read < buf_size) {
    read = recv(fd, buf_start + curr_read, buf_size - curr_read, 0);
    if (read > 0) {
      curr_read += read;
    } else if (read == 0) {
      // exit
      *exit = true;
      return curr_read;
    } else if (read == -1 && errno == EINTR) {
      // normal interrupt
      continue;
    } else if (read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      // real all
      break;
    } else {
      // problem occurs
      *exit = true;
      break;
    }
  }
  return curr_read;
}