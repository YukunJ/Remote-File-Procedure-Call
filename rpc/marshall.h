/**
 * @file marshall.h
 * @author Yukun J
 * @init_date Jan 20 2023
 *
 * This file includes the marshalling functionality between client and server
 * to support argument packing and unpacking
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MARSHALL_H
#define MARSHALL_H

/* a list of system calls allowed for marshall */
#define OPEN_OP 0
#define CLOSE_OP 1
#define READ_OP 2
#define WRITE_OP 3
#define LSEEK_OP 4
#define STAT_OP 5
#define UNLINK_OP 6
#define GETDIRENTRIES_OP 7
#define GETDIRTREE_OP 8
#define FREEDIRTREE_OP 9

/* a few headers in the marshall rpc request and response */
#define HEADER_COMMAND "Command"
#define HEADER_ERRNO "Errno"
#define HEADER_RETURN_SIZE "ReturnSize"
#define HEADER_PARAM "Param"
#define HEADER_PARAM_NUM "ParamNum"

/* the offset to be added/subtracted dealing with local and remote file
 * descriptor */
#define OFFSET 12345

/* the colon between a header and its value */
#if !defined(COLON)
#define COLON ":"
#endif

/* the line splitter in serialized rpc request and response */
#define LINE_SPLIT "\r\n"

/* the buf size to temporarily store some bytes value */
#define TEMP_BUF_SIZE 1024

/* the struct for rpc request */
typedef struct {
  int command_op;      /* the procedure to call */
  int param_num;       /* how many parameters are packed */
  size_t* param_sizes; /* size of each param in bytes */
  char** params;       /* pointer to each param in bytes form */
} rpc_request;

/* the struct for rpc response */
typedef struct {
  int errno_num;
  size_t return_size;
  char* return_val;
} rpc_response;

/**
 * @brief factory method for an rpc_request
 * it initialize the command_op and param_num in the struct
 * and also allocate space for the param_sizes and params fields
 * @param command_op the command option
 * @param param_num how many params to be packed
 * @return a dynamically allocated rpc_request struct
 */
rpc_request* init_request(int command_op, int param_num);

/**
 * @brief free the space allocated for the request
 *        including the space for params nested
 * @param request pointer to rpc_request struct
 */
void free_request(rpc_request* request);

/**
 * @brief serialize the rpc request into a char buffer stream
 *        the rpc request should already be fully packed ready
 * @param request pointer to rpc_request struct
 * @param buf the place to serialize to, assume the buffer is large enough
 * @return how many bytes are serialized
 */
size_t serialize_request(rpc_request* request, char* buf);

/**
 * @brief from a stream of bytes to reconstruct the rpc_request struct
 * @param buf the beginning of stream containing rpc_request (assume big enough)
 * @return pointer to a re-constructed rpc_request struct
 */
rpc_request* deserialize_request(char* buf);

/**
 * @brief pack an integral type into the rpc_request struct
 *        use size_t since it should be long enough
 * @param request the pointer to rpc_request struct
 * @param offset which param it is in the rpc_request
 * @param val the value to be packed
 */
void pack_integral(rpc_request* request, int offset, size_t val);

/**
 * @brief pack a byte stream into the rpc_request struct
 * @param request the pointer to rpc_request struct
 * @param offset which param it is in the rpc_request
 * @param buf the beginning of the byte stream
 * @param buf_size how long is this byte stream
 */
void pack_pointer(rpc_request* request, int offset, const char* buf,
                  size_t buf_size);

/* debug purpose to print out an rpc_request struct */
void print_request(rpc_request* request);

/**
 * @brief serialize the rpc response into a char buffer stream
 *        the rpc response should already be fully packed ready
 * @param response pointer to rpc_response struct
 * @param buf the place to serialize to, assume the buffer is large enough
 * @return how many bytes are serialized
 */
size_t serialize_response(rpc_response* response, char* buf);

/**
 * @brief from a stream of bytes to reconstruct the rpc_response struct
 * @param buf the beginning of stream containing rpc_response (assume big
 * enough)
 * @return pointer to a re-constructed rpc_response struct
 */
rpc_response* deserialize_response(char* buf);

/**
 * @brief create a rpc_response struct based on an integral return value
 * @param errno_num the errno
 * @param return_value the integral return value
 * @return pointer to an allocated rpc_response struct
 */
rpc_response* make_integral_response(int errno_num, size_t return_value);

/**
 * @brief create a rpc_response struct base on a char stream return value
 * @param errno_num the errno
 * @param buf the beginning of char stream
 * @param buf_size size of the char stream
 * @return pointer to an allocated rpc_response struct
 */
rpc_response* make_pointer_response(int errno_num, char* buf, size_t buf_size);

/**
 * @brief free the space allocated for the rpc response
 * @param response pointer to the rpc_response struct
 */
void free_response(rpc_response* response);

/* debug purpose to print out an rpc_response struct */
void print_response(rpc_response* response);

#endif  // MARSHALL_H
