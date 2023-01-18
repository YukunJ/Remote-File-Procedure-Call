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

/* the local buffer size for each read from socket */
#define BUF_SIZE 1024

/* thread service main entry */
void *service(void *arg) {
  char buf[BUF_SIZE + 1];
  memset(buf, 0, sizeof(buf));
  int client_fd = (int)(intptr_t)arg;
  bool client_exit = false;
  ssize_t read = robust_read(client_fd, buf, BUF_SIZE, &client_exit);
  buf[read] = '\0';
  if (read) {
    printf("%s\n", buf);
  }
  if (client_exit) {
    close(client_fd);
  }
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