/**
 * @file server.c
 * @author Yukun J
 * @init_date Jan 18 2023
 *
 * This file implements a remote file RPC server
 * which is able to concurrently serving multiple client requests
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include "marshall.h"
#include "socket.h"

/* server the 'open' rpc request */
void serve_open(int client_fd, rpc_request *request) {
  int old_errno = errno;
  errno = 0;
  int flags = atoi(request->params[1]);
  mode_t mode = atoi(request->params[2]);
  int fd = open(request->params[0], flags, mode);
  if (fd != -1) {
    fd += OFFSET;
  }
  /* prepare rpc response */
  rpc_response *response = make_integral_response(errno, fd);
  errno = old_errno;
  char *stream = (char *)calloc(STORAGE_SIZE, sizeof(char));
  size_t response_size = serialize_response(response, stream);
  free(response);
  /* send the response back to client */
  send_message(client_fd, stream, response_size);
  free(stream);
}

/* thread service main entry */
void *service(void *arg) {
  char *storage_buf = (char *)calloc(STORAGE_SIZE + 1, sizeof(char));
  size_t storage_size = 0;
  int client_fd = (int)(intptr_t)arg;
  bool client_exit = false;
  while (true) {
    ssize_t read = greedy_read(client_fd, storage_buf + storage_size,
                               STORAGE_SIZE - storage_size, &client_exit);
    if (read >= 0 && storage_size + read < STORAGE_SIZE) {
      if (read > 0) {
        storage_size += read;
      }
    } else {
      fprintf(stderr,
              "robust_read returns read=%zd and current storage_size=%zd\n",
              read, storage_size);
    }
    char *message;
    while ((message = parse_message(storage_buf, &storage_size)) != NULL) {
      // successfully parse a message, parse it into rpc request struct
      rpc_request *request = deserialize_request(message);
      switch (request->command_op) {
        case (OPEN_OP):
          serve_open(client_fd, request);
          break;
        default:
          fprintf(stderr, "Unknown command option:%d\n", request->command_op);
      }
      free(request);
      free(message);
    }
    if (client_exit) {
      close(client_fd);
      break;
    }
  }
  free(storage_buf);
  return NULL;
}

int main(int argc, char *argv[]) {
  int listen_fd = build_server();
  while (true) {
    int client_fd = accept_client(listen_fd);
    if (client_fd != -1) {
      // non-blocking mode
      fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
      service((void *)(intptr_t)client_fd);
    }
  }
  close(listen_fd);
  return 0;
}