/**
 * @file server.c
 * @author Yukun J
 * @init_date Jan 18 2023
 *
 * This file implements a remote file RPC server
 * which is able to concurrently serving multiple client requests
 */

#include <stdint.h>
#include <stdio.h>

#include "socket.h"

/* storage buffer to store all bytes received from the socket, 1MB by default */
#define STORAGE_SIZE (1024 * 1024)

/* thread service main entry */
void *service(void *arg) {
  char *storage_buf = (char *)calloc(STORAGE_SIZE + 1, sizeof(char));
  size_t storage_size = 0;
  int client_fd = (int)(intptr_t)arg;
  bool client_exit = false;
  while (true) {
    ssize_t read =
        robust_read(client_fd, storage_buf, STORAGE_SIZE, &client_exit);
    if (read > 0 && storage_size + read < STORAGE_SIZE) {
      storage_size += read;
    } else {
      fprintf(stderr,
              "robust_read returns read=%zd and current storage_size=%zd\n",
              read, storage_size);
    }
    char *message;
    while ((message = parse_message(storage_buf, &storage_size)) != NULL) {
      // successfully parse a message, process it
      printf("%s\n", message);
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
      service((void *)(intptr_t)client_fd);
    }
  }
  close(listen_fd);
  return 0;
}