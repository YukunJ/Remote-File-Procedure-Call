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
#include <sys/wait.h>

#include "marshall.h"
#include "socket.h"

/**
 * @brief serialize a rpc_response struct and send it over network
 *        the response is freed inside this func
 * @param fd client fd
 * @param response pointer to a rpc_response struct
 * @param stream a long enough buffer to be streamed into
 */
void send_response(int fd, rpc_response *response, char *stream) {
  /* serialize the response into stream */
  memset(stream, 0, STORAGE_SIZE + 1);
  size_t response_size = serialize_response(response, stream);
  free(response);
  /* send the response back to client */
  send_message(fd, stream, response_size);
}

/* serve the 'open' rpc request */
void serve_open(int client_fd, rpc_request *request, char *stream) {
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
  send_response(client_fd, response, stream);
}

/* server the 'close' rpc request */
void serve_close(int client_fd, rpc_request *request, char *stream) {
  int old_errno = errno;
  errno = 0;
  int fd = atoi(request->params[0]) - OFFSET;  // convert into local fd
  int return_val = close(fd);
  rpc_response *response = make_integral_response(errno, return_val);
  errno = old_errno;
  send_response(client_fd, response, stream);
}

/* server the 'write' rpc request */
void serve_write(int client_fd, rpc_request *request, char *stream) {
  int old_errno = errno;
  errno = 0;
  int fd = atoi(request->params[0]) - OFFSET;  // convert into local fd
  size_t count = atol(request->params[2]);
  ssize_t return_val = write(fd, request->params[1], count);
  rpc_response *response = make_integral_response(errno, return_val);
  errno = old_errno;
  send_response(client_fd, response, stream);
}

/* thread service main entry */
void *service(void *arg) {
  /* each client get two designated big buffers */
  char *storage_buf = (char *)calloc(STORAGE_SIZE + 1, sizeof(char));
  char *stream = (char *)calloc(STORAGE_SIZE + 1, sizeof(char));
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
              "greedy_read returns read=%zd and current storage_size=%zd\n",
              read, storage_size);
    }
    char *message;
    while ((message = parse_message(storage_buf, &storage_size)) != NULL) {
      // successfully parse a message, parse it into rpc request struct
      rpc_request *request = deserialize_request(message);
      switch (request->command_op) {
        case (OPEN_OP):
          serve_open(client_fd, request, stream);
          break;
        case (CLOSE_OP):
          serve_close(client_fd, request, stream);
          break;
        case (WRITE_OP):
          serve_write(client_fd, request, stream);
          break;
        default:
          fprintf(stderr, "Unknown command option:%d\n", request->command_op);
          print_request(request);
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
  free(stream);
  return NULL;
}

int main(int argc, char *argv[]) {
  int listen_fd = build_server();
  while (true) {
    int client_fd = accept_client(listen_fd);
    if (client_fd != -1) {
      pid_t id = fork();
      if (id < 0) {
        fprintf(stderr, "fork() < 0 error");
        break;
      }
      if (id == 0) {
        // child
        close(listen_fd);
        // non-blocking mode
        fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
        service((void *)(intptr_t)client_fd);
        return 0;
      } else {
        // parent
        close(client_fd);
        // try harvest any available zombie child processes
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {
        }
      }
    }
  }
  // main server thread clean up
  close(listen_fd);
  return 0;
}