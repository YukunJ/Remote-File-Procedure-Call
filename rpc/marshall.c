/**
 * @file marshall.c
 * @author Yukun J
 * @init_date Jan 20 2023
 *
 * This is the implementation file for marshall.h. See details there.
 */

#include "marshall.h"

/**
 * @brief factory method for an rpc_request
 * it initialize the command_op and param_num in the struct
 * and also allocate space for the param_sizes and params fields
 * @param command_op the command option
 * @param param_num how many params to be packed
 * @return a dynamically allocated rpc_request struct
 */
rpc_request* init_request(int command_op, int param_num) {
  rpc_request* request = (rpc_request*)malloc(sizeof(rpc_request));
  request->command_op = command_op;
  request->param_num = param_num;
  request->param_sizes = (size_t*)calloc(param_num, sizeof(size_t));
  request->params = (char**)calloc(param_num, sizeof(char*));
  return request;
}

/**
 * @brief free the space allocated for the request
 *        including the space for params nested
 * @param request pointer to rpc_request struct
 */
void free_request(rpc_request* request) {
  for (int i = 0; i < request->param_num; i++) {
    free(request->params[i]);
  }
  free(request->params);
  free(request->param_sizes);
  free(request);
}

/**
 * @brief serialize the rpc request into a char buffer stream
 *        the rpc request should already be fully packed ready
 * @param request pointer to rpc_request struct
 * @param buf the place to serialize to, assume the buffer is large enough
 * @return how many bytes are serialized
 */
size_t serialize_request(rpc_request* request, char* buf) {
  size_t request_size = 0;
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));
  size_t temp_len;
  char* buf_ptr = buf;

  /* the command option */
  sprintf(temp_buf, "%s%s%d%s", HEADER_COMMAND, COLON, request->command_op,
          LINE_SPLIT);
  temp_len = strlen(temp_buf);
  memcpy(buf_ptr, temp_buf, temp_len);
  buf_ptr += temp_len;
  request_size += temp_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* the param num */
  sprintf(temp_buf, "%s%s%d%s", HEADER_PARAM_NUM, COLON, request->param_num,
          LINE_SPLIT);
  temp_len = strlen(temp_buf);
  memcpy(buf_ptr, temp_buf, temp_len);
  buf_ptr += temp_len;
  request_size += temp_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* the param size and param each take one line, back by back */
  for (int offset = 0; offset < request->param_num; offset++) {
    /* first line is the param size */
    sprintf(temp_buf, "%zu%s", request->param_sizes[offset], LINE_SPLIT);
    temp_len = strlen(temp_buf);
    memcpy(buf_ptr, temp_buf, temp_len);
    buf_ptr += temp_len;
    request_size += temp_len;
    memset(temp_buf, 0, sizeof(temp_buf));

    /* next line is the real param */
    memcpy(buf_ptr, request->params[offset], request->param_sizes[offset]);
    buf_ptr += request->param_sizes[offset];
    request_size += request->param_sizes[offset];
    memcpy(buf_ptr, LINE_SPLIT, strlen(LINE_SPLIT));
    buf_ptr += strlen(LINE_SPLIT);
    request_size += strlen(LINE_SPLIT);
  }

  return request_size;
}

/**
 * @brief from a stream of bytes to reconstruct the rpc_request struct
 * @param buf the beginning of stream containing rpc_request (assume big enough)
 * @return pointer to a re-constructed rpc_request struct
 */
rpc_request* deserialize_request(char* buf) {
  rpc_request* request = (rpc_request*)malloc(sizeof(rpc_request));
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));
  char* buf_ptr = buf;
  char* line_end;
  char* colon;
  /* parse the command op line */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  sscanf(temp_buf, "%d", &request->command_op);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* parse the param num */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  sscanf(temp_buf, "%d", &request->param_num);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* allocate space for rpc_request struct */
  size_t param_num = request->param_num;
  request->param_sizes = (size_t*)calloc(param_num, sizeof(size_t));
  request->params = (char**)calloc(param_num, sizeof(char*));

  /* parse each param, each spans two lines.
     first line is len, next line is actual content */
  for (int offset = 0; offset < request->param_num; offset++) {
    /* scan the param size */
    line_end = strstr(buf_ptr, LINE_SPLIT);
    memcpy(temp_buf, buf_ptr, line_end - buf_ptr);
    sscanf(temp_buf, "%zu", &request->param_sizes[offset]);
    memset(temp_buf, 0, sizeof(temp_buf));
    size_t this_param_size = request->param_sizes[offset];
    buf_ptr = line_end + strlen(LINE_SPLIT);
    /* allocate space for this param */
    request->params[offset] = (char*)calloc(this_param_size + 1, sizeof(char));
    /* scan the real param */
    memcpy(request->params[offset], buf_ptr, this_param_size);
    buf_ptr += this_param_size;     // skip this param
    buf_ptr += strlen(LINE_SPLIT);  // skip line splitter
  }

  return request;
}

/**
 * @brief pack an integral type into the rpc_request struct
 *        use size_t since it should be long enough
 * @param request the pointer to rpc_request struct
 * @param offset which param it is in the rpc_request
 * @param val the value to be packed
 */
void pack_integral(rpc_request* request, int offset, ssize_t val) {
  assert(offset < request->param_num);
  char integral_buf[TEMP_BUF_SIZE];
  memset(integral_buf, 0, sizeof(integral_buf));
  sprintf(integral_buf, "%ld", val);
  size_t line_len = strlen(integral_buf);
  request->param_sizes[offset] = line_len;
  request->params[offset] = (char*)calloc(line_len + 1, sizeof(char));
  memcpy(request->params[offset], integral_buf, line_len);
}

/**
 * @brief pack a byte stream into the rpc_request struct
 * @param request the pointer to rpc_request struct
 * @param offset which param it is in the rpc_request
 * @param buf the beginning of the byte stream
 * @param buf_size how long is this byte stream
 */
void pack_pointer(rpc_request* request, int offset, const char* buf,
                  size_t buf_size) {
  assert(offset < request->param_num);
  request->param_sizes[offset] = buf_size;
  request->params[offset] = (char*)calloc(buf_size + 1, sizeof(char));
  memcpy(request->params[offset], buf, buf_size);
}

/* debug purpose to print out an rpc_request struct */
void print_request(rpc_request* request) {
  printf("========RPC Request========\n");
  printf("%s: %d\n", HEADER_COMMAND, request->command_op);
  printf("%s: %d\n", HEADER_PARAM_NUM, request->param_num);
  for (int i = 0; i < request->param_num; i++) {
    printf("%s%d: size=%zu content=%s\n", HEADER_PARAM, i + 1,
           request->param_sizes[i], request->params[i]);
  }
  printf("===========End==========\n");
}

/**
 * @brief serialize the rpc response into a char buffer stream
 *        the rpc response should already be fully packed ready
 * @param response pointer to rpc_response struct
 * @param buf the place to serialize to, assume the buffer is large enough
 * @return how many bytes are serialized
 */
size_t serialize_response(rpc_response* response, char* buf) {
  size_t response_size = 0;
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));
  size_t temp_len;
  char* buf_ptr = buf;

  /* the errno */
  sprintf(temp_buf, "%s%s%d%s", HEADER_ERRNO, COLON, response->errno_num,
          LINE_SPLIT);
  temp_len = strlen(temp_buf);
  memcpy(buf_ptr, temp_buf, temp_len);
  buf_ptr += temp_len;
  response_size += temp_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* the return size */
  sprintf(temp_buf, "%s%s%zu%s", HEADER_RETURN_SIZE, COLON,
          response->return_size, LINE_SPLIT);
  temp_len = strlen(temp_buf);
  memcpy(buf_ptr, temp_buf, temp_len);
  buf_ptr += temp_len;
  response_size += temp_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* the actual return value */
  memcpy(buf_ptr, response->return_val, response->return_size);
  buf_ptr += response->return_size;
  response_size += response->return_size;
  memcpy(buf_ptr, LINE_SPLIT, strlen(LINE_SPLIT));
  buf_ptr += strlen(LINE_SPLIT);
  response_size += strlen(LINE_SPLIT);

  return response_size;
}

/**
 * @brief from a stream of bytes to reconstruct the rpc_response struct
 * @param buf the beginning of stream containing rpc_response (assume big
 * enough)
 * @return pointer to a re-constructed rpc_response struct
 */
rpc_response* deserialize_response(char* buf) {
  rpc_response* response = (rpc_response*)malloc(sizeof(rpc_response));
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));
  char* buf_ptr = buf;
  char* line_end;
  char* colon;
  /* parse the errno line */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  sscanf(temp_buf, "%d", &response->errno_num);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* parse the return size line */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  sscanf(temp_buf, "%zu", &response->return_size);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* allocate space for return value */
  response->return_val = (char*)calloc(response->return_size + 1, sizeof(char));

  /* parse the actual return value */
  memcpy(response->return_val, buf_ptr, response->return_size);

  return response;
}

/**
 * @brief create a rpc_response struct based on an integral return value
 * @param errno_num the errno
 * @param return_value the integral return value
 * @return pointer to an allocated rpc_response struct
 */
rpc_response* make_integral_response(int errno_num, ssize_t return_value) {
  rpc_response* response = (rpc_response*)malloc(sizeof(rpc_response));
  response->errno_num = errno_num;
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));
  sprintf(temp_buf, "%ld", return_value);
  size_t return_size = strlen(temp_buf);
  response->return_size = return_size;
  response->return_val = (char*)calloc(return_size + 1, sizeof(char));
  memcpy(response->return_val, temp_buf, return_size);
  return response;
}

/**
 * @brief create a rpc_response struct base on a char stream return value
 * @param errno_num the errno
 * @param buf the beginning of char stream
 * @param buf_size size of the char stream
 * @return pointer to an allocated rpc_response struct
 */
rpc_response* make_pointer_response(int errno_num, char* buf, size_t buf_size) {
  rpc_response* response = (rpc_response*)malloc(sizeof(rpc_response));
  response->errno_num = errno_num;
  response->return_size = buf_size;
  response->return_val = (char*)calloc(buf_size + 1, sizeof(char));
  memcpy(response->return_val, buf, buf_size);
  return response;
}

/**
 * @brief free the space allocated for the rpc response
 * @param response pointer to the rpc_response struct
 */
void free_response(rpc_response* response) {
  free(response->return_val);
  free(response);
}

/* debug purpose to print out an rpc_response struct */
void print_response(rpc_response* response) {
  printf("========RPC Response========\n");
  printf("%s: %d\n", HEADER_ERRNO, response->errno_num);
  printf("%s: %zu\n", HEADER_RETURN_SIZE, response->return_size);
  printf("%s\n", response->return_val);
  printf("===========End==========\n");
}