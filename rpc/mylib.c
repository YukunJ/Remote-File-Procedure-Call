/**
 * @file mylib.c
 * @author Yukun J
 * @init_date Jan 18 2023
 *
 * This file is the client stub for the file RPC, which replaces
 * a few 'local' file operation calls with the stub version which
 * sends request to the remote server and expect response
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>

#include "marshall.h"
#include "socket.h"

/* a list of system calls to interpose */
#define OPEN_COMMAND "open"
#define CLOSE_COMMAND "close"
#define READ_COMMAND "read"
#define WRITE_COMMAND "write"
#define LSEEK_COMMAND "lseek"
#define STAT_COMMAND "stat"
#define UNLINK_COMMAND "unlink"
#define GETDIRENTRIES_COMMAND "getdirentries"
#define GETDIRTREE_COMMAND "getdirtree"
#define FREEDIRTREE_COMMAND "freedirtree"

/* the server socket to be build once only in init function */
int server_fd;

// The following line declares a function pointer with the same prototype as the
// open function.
int (*orig_open)(const char *pathname, int flags,
                 ...);  // mode_t mode is needed when flags includes O_CREAT

int (*orig_close)(int fd);

ssize_t (*orig_read)(int fd, void *buf, size_t count);

ssize_t (*orig_write)(int fd, const void *buf, size_t count);

off_t (*orig_lseek)(int fd, off_t offset, int whence);

int (*orig_stat)(const char *restrict pathname, struct stat *restrict statbuf);

int (*orig_unlink)(const char *pathname);

ssize_t (*orig_getdirentries)(int fd, char *buf, size_t nbytes, off_t *basep);

struct dirtreenode *(*orig_getdirtree)(const char *path);

void (*orig_freedirtree)(struct dirtreenode *dt);

/**
 * @brief wait for a full message to arrive and parse into rpc_response struct
 *        this is a blocking call
 * @param fd the server fd
 * @return a pointer to an rpc_response
 */
rpc_response *wait_response(int fd) {
  char *storage_buf = (char *)calloc(STORAGE_SIZE + 1, sizeof(char));
  size_t storage_size = 0;
  char *message = NULL;
  while (message == NULL) {
    bool server_exit = false;
    ssize_t read = greedy_read(fd, storage_buf + storage_size,
                               STORAGE_SIZE - storage_size, &server_exit);
    if (read >= 0 && storage_size + read <= STORAGE_SIZE) {
      storage_size += read;
    } else {
      fprintf(stderr,
              "greedy_read returns read=%zd and current storage_size=%zd\n",
              read, storage_size);
    }
    message = parse_message(storage_buf, &storage_size);
    if (message) {
      rpc_response *response = deserialize_response(message);
      free(storage_buf);
      free(message);
      return response;
    }
  }
  free(storage_buf);
  return NULL;
}

// This is our replacement for the open function from libc.
int open(const char *pathname, int flags, ...) {
  mode_t m = 0;
  if (flags & O_CREAT) {
    va_list a;
    va_start(a, flags);
    m = va_arg(a, mode_t);
    va_end(a);
  }
  // we just print a message, then call through to the original open function
  // (from libc)
  fprintf(stderr, "mylib: open called for path %s\n", pathname);

  // prepare the rpc request
  rpc_request *request = init_request(OPEN_OP, 3);
  pack_pointer(request, 0, pathname, strlen(pathname));
  pack_integral(request, 1, flags);
  pack_integral(request, 2, m);

  // serialize the rpc request and send it through network
  char *stream = (char *)calloc(STORAGE_SIZE, sizeof(char));
  size_t rpc_stream_size = serialize_request(request, stream);
  free(request);

  // send the rpc request to server
  send_message(server_fd, stream, rpc_stream_size);
  free(stream);

  // wait for response from the server, blocking
  rpc_response *response = wait_response(server_fd);
  int remote_fd = atoi(response->return_val);
  if (remote_fd < 0) {
    errno = response->errno_num;
  }
  free(response);

  return remote_fd;
}

// This is our replacement for the close function from libc.
int close(int fd) {
  fprintf(stderr, "mylib: close called for fd=%d\n", fd);
  sleep(1);
  // send_message(server_fd, CLOSE_COMMAND, strlen(CLOSE_COMMAND));
  return orig_close(fd);
}

// This is our replacement for the read function from libc.
ssize_t read(int fd, void *buf, size_t count) {
  fprintf(stderr, "mylib: read called for fd=%d\n", fd);
  // send_message(server_fd, READ_COMMAND, strlen(READ_COMMAND));
  return orig_read(fd, buf, count);
}

// This is our replacement for the write function from libc.
ssize_t write(int fd, const void *buf, size_t count) {
  fprintf(stderr, "mylib: write called for fd=%d\n", fd);
  sleep(1);
  // send_message(server_fd, WRITE_COMMAND, strlen(WRITE_COMMAND));
  return orig_write(fd, buf, count);
}

// This is our replacement for the lseek function from libc.
off_t lseek(int fd, off_t offset, int whence) {
  fprintf(stderr, "mylib: lseek called for fd=%d\n", fd);
  // send_message(server_fd, LSEEK_COMMAND, strlen(LSEEK_COMMAND));
  return orig_lseek(fd, offset, whence);
}

// This is our replacement for the stat function from libc.
int stat(const char *restrict pathname, struct stat *restrict statbuf) {
  fprintf(stderr, "mylib: stat called for pathname=%s\n", pathname);
  // send_message(server_fd, STAT_COMMAND, strlen(STAT_COMMAND));
  return orig_stat(pathname, statbuf);
}

// This is our replacement for the unlink function from libc.
int unlink(const char *pathname) {
  fprintf(stderr, "mylib: unlink called for pathname=%s\n", pathname);
  // send_message(server_fd, UNLINK_COMMAND, strlen(UNLINK_COMMAND));
  return orig_unlink(pathname);
}

// This is our replacement for the getdirentries function from libc.
ssize_t getdirentries(int fd, char *buf, size_t nbytes, off_t *basep) {
  fprintf(stderr, "mylib: getdirentries called for fd=%d\n", fd);
  // send_message(server_fd, GETDIRENTRIES_COMMAND,
  // strlen(GETDIRENTRIES_COMMAND));
  return orig_getdirentries(fd, buf, nbytes, basep);
}

// This is our replacement for the getdirtree from our own local implementation.
struct dirtreenode *getdirtree(const char *path) {
  fprintf(stderr, "mylib: getdirtree called for path=%s\n", path);
  // send_message(server_fd, GETDIRTREE_COMMAND, strlen(GETDIRTREE_COMMAND));
  return orig_getdirtree(path);
}

// This is our replacement for the freedirtree from our own local
// implementation.
void freedirtree(struct dirtreenode *dt) {
  fprintf(stderr, "mylib: freedirtree called");
  // send_message(server_fd, FREEDIRTREE_COMMAND, strlen(FREEDIRTREE_COMMAND));
  return orig_freedirtree(dt);
}

// This function is automatically called when program is started
void _init(void) {
  // set function pointer orig_open to point to the original open function
  fprintf(stderr, "Init mylib for library interposition\n");
  server_fd = build_client();
  // non-blocking mode
  fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK);
  if (server_fd < 0) {
    fprintf(stderr, "Fail to build connection with server_fd\n");
    exit(EXIT_FAILURE);
  }
  orig_open = dlsym(RTLD_NEXT, OPEN_COMMAND);
  orig_close = dlsym(RTLD_NEXT, CLOSE_COMMAND);
  orig_read = dlsym(RTLD_NEXT, READ_COMMAND);
  orig_write = dlsym(RTLD_NEXT, WRITE_COMMAND);
  orig_lseek = dlsym(RTLD_NEXT, LSEEK_COMMAND);
  orig_stat = dlsym(RTLD_NEXT, STAT_COMMAND);
  orig_unlink = dlsym(RTLD_NEXT, UNLINK_COMMAND);
  orig_getdirentries = dlsym(RTLD_NEXT, GETDIRENTRIES_COMMAND);
  orig_getdirtree = dlsym(RTLD_NEXT, GETDIRTREE_COMMAND);
  orig_freedirtree = dlsym(RTLD_NEXT, FREEDIRTREE_COMMAND);
}
