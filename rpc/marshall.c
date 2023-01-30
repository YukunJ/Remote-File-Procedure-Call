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
 * @brief factory method for an rpc_response
 * it initialize the errno_num and return_num in the struct
 * and also allocate space for the return_sizes and return_vals array
 * @param errno_num errno
 * @param return_num how many return values
 * @return a dynamically allocated rpc_response struct
 */
rpc_response* init_response(int errno_num, int return_num) {
  rpc_response* response = (rpc_response*)malloc(sizeof(rpc_response));
  response->errno_num = errno_num;
  response->return_num = return_num;
  response->return_sizes = (size_t*)calloc(return_num, sizeof(size_t));
  response->return_vals = (char**)calloc(return_num, sizeof(char*));
  return response;
}

/**
 * @brief free the space allocated for the response
 *        including the space for params nested
 * @param response pointer to rpc_response struct
 */
void free_response(rpc_response* response) {
  for (int i = 0; i < response->return_num; i++) {
    free(response->return_vals[i]);
  }
  free(response->return_vals);
  free(response->return_sizes);
  free(response);
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

  /* the return num */
  sprintf(temp_buf, "%s%s%d%s", HEADER_RETURN_NUM, COLON, response->return_num,
          LINE_SPLIT);
  temp_len = strlen(temp_buf);
  memcpy(buf_ptr, temp_buf, temp_len);
  buf_ptr += temp_len;
  response_size += temp_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* the return size and return val each take one line, back by back */
  for (int offset = 0; offset < response->return_num; offset++) {
    /* first line is the return size */
    sprintf(temp_buf, "%zu%s", response->return_sizes[offset], LINE_SPLIT);
    temp_len = strlen(temp_buf);
    memcpy(buf_ptr, temp_buf, temp_len);
    buf_ptr += temp_len;
    response_size += temp_len;
    memset(temp_buf, 0, sizeof(temp_buf));

    /* next line is the read param */
    memcpy(buf_ptr, response->return_vals[offset],
           response->return_sizes[offset]);
    buf_ptr += response->return_sizes[offset];
    response_size += response->return_sizes[offset];
    memcpy(buf_ptr, LINE_SPLIT, strlen(LINE_SPLIT));
    buf_ptr += strlen(LINE_SPLIT);
    response_size += strlen(LINE_SPLIT);
  }

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

  /* parse the return num line */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  sscanf(temp_buf, "%d", &response->return_num);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* allocate space for return sizes and return vals */
  int return_num = response->return_num;
  response->return_sizes = (size_t*)calloc(return_num, sizeof(size_t));
  response->return_vals = (char**)calloc(return_num, sizeof(char*));

  /* parse the actual return value
     first line is return size, second line is actual content */
  for (int offset = 0; offset < return_num; offset++) {
    /* scan the return size */
    line_end = strstr(buf_ptr, LINE_SPLIT);
    memcpy(temp_buf, buf_ptr, line_end - buf_ptr);
    sscanf(temp_buf, "%zu", &response->return_sizes[offset]);
    memset(temp_buf, 0, sizeof(temp_buf));
    size_t this_return_size = response->return_sizes[offset];
    buf_ptr = line_end + strlen(LINE_SPLIT);
    /* allocate space for this return */
    response->return_vals[offset] =
        (char*)calloc(this_return_size + 1, sizeof(char));
    /* scan the real return value */
    memcpy(response->return_vals[offset], buf_ptr, this_return_size);
    buf_ptr += this_return_size;    // skip this return content
    buf_ptr += strlen(LINE_SPLIT);  // skip line splitter
  }
  return response;
}

/**
 * @brief marshall an integral type into the rpc_response struct
 *        use size_t since it should be long enough
 * @param request the pointer to rpc_response struct
 * @param offset which param it is in the rpc_response
 * @param val the value to be packed
 */
void marshall_integral(rpc_response* response, int offset, ssize_t val) {
  assert(offset < response->return_num);
  char integral_buf[TEMP_BUF_SIZE];
  memset(integral_buf, 0, sizeof(integral_buf));
  sprintf(integral_buf, "%ld", val);
  size_t line_len = strlen(integral_buf);
  response->return_sizes[offset] = line_len;
  response->return_vals[offset] = (char*)calloc(line_len + 1, sizeof(char));
  memcpy(response->return_vals[offset], integral_buf, line_len);
}

/**
 * @brief marshall a byte stream into the rpc_response struct
 * @param response the pointer to rpc_response struct
 * @param offset which param it is in the rpc_response
 * @param buf the beginning of the byte stream
 * @param buf_size how long is this byte stream
 */
void marshall_pointer(rpc_response* response, int offset, const char* buf,
                      size_t buf_size) {
  assert(offset < response->return_num);
  response->return_sizes[offset] = buf_size;
  response->return_vals[offset] = (char*)calloc(buf_size + 1, sizeof(char));
  memcpy(response->return_vals[offset], buf, buf_size);
}

/**
 * @brief marshall a recursive dirtreenode struct into a byte buffer
 * @param root pointer to the root node of this dirtree
 * @param buf a big enough buffer to hold the serialized dirtree
 * @return how many bytes are written into buf by this serialization operations
 */
size_t serialize_dirtree(struct dirtreenode* root, char* buf) {
  char* buf_begin = buf;
  char* buf_end = serialize_node(root, buf);
  return (size_t)(buf_end - buf_begin);
}

/**
 * @brief helper function called in 'serialize_dirtree'
 *       this serialize a single dirtreenode and recursively call it self
 * @param node pointer to the dirtreenode to be serialized
 * @param buf the buffer to serialize into
 * @return next position of the buffer after serialization
 */
char* serialize_node(struct dirtreenode* node, char* buf) {
  if (node == NULL) {
    // null node, nothing to serialize
    return buf;
  }
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));

  /* name of the tree node */
  sprintf(temp_buf, "%s%s%s%s", HEADER_TREE_NAME, COLON, node->name,
          LINE_SPLIT);
  size_t line_len = strlen(temp_buf);
  memcpy(buf, temp_buf, line_len);
  buf += line_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* number of children of tree node */
  sprintf(temp_buf, "%s%s%d%s", HEADER_TREE_CHILD_NUM, COLON, node->num_subdirs,
          LINE_SPLIT);
  line_len = strlen(temp_buf);
  memcpy(buf, temp_buf, line_len);
  buf += line_len;
  memset(temp_buf, 0, sizeof(temp_buf));

  /* serialize each child recursively */
  int child_num = node->num_subdirs;
  for (int i = 0; i < child_num; i++) {
    buf = serialize_node(node->subdirs[i], buf);
  }

  /* return the final position of the buf */
  return buf;
}

/**
 * @brief unmarshall a recursive dirtreenode struct from a stream buffer
 * @param buf the buffer containing stream
 * @return pointer to a dynamically allocated dirtreenode
 */
struct dirtreenode* deserialize_dirtree(char* buf) {
  return deserialize_node(&buf);
}

/**
 * @brief helper function called in `deserialize_dirtree`
 *        this deserialize a single dirtreenode and recursively all its children
 * @param buf to be modified on the fly to reflect how far we go in the stream
 * buffer
 * @return a dynamically allocated single dirtreenode with all its children
 * constructed
 */
struct dirtreenode* deserialize_node(char** buf) {
  struct dirtreenode* node =
      (struct dirtreenode*)calloc(sizeof(struct dirtreenode), sizeof(char));
  char temp_buf[TEMP_BUF_SIZE];
  memset(temp_buf, 0, sizeof(temp_buf));
  char* buf_ptr = *buf;
  char* line_end;
  char* colon;

  /* parse the name line */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  size_t name_len = strlen(temp_buf);
  node->name =
      (char*)calloc(name_len + 1, sizeof(char));  // allocate space for name
  memcpy(node->name, temp_buf, name_len);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* parse the child num line */
  line_end = strstr(buf_ptr, LINE_SPLIT);
  colon = strstr(buf_ptr, COLON);
  assert(colon < line_end);
  memcpy(temp_buf, colon + 1, line_end - colon - 1);
  sscanf(temp_buf, "%d", &node->num_subdirs);
  memset(temp_buf, 0, sizeof(temp_buf));
  buf_ptr = line_end + strlen(LINE_SPLIT);

  /* allocate space for num children struct pointer */
  node->subdirs = (struct dirtreenode**)calloc(node->num_subdirs,
                                               sizeof(struct dirtreenode*));

  /* recursively parse child */
  for (int i = 0; i < node->num_subdirs; i++) {
    node->subdirs[i] = deserialize_node(&buf_ptr);
  }
  *buf = buf_ptr;
  return node;
}

/**
 * @brief recursively release memory allocated for the node and all its children
 * @param root pointer to the root dirtreenode
 */
void free_dirtreenode(struct dirtreenode* root) {
  if (root != NULL) {
    free(root->name);
    for (int i = 0; i < root->num_subdirs; i++) {
      free_dirtreenode(root->subdirs[i]);
    }
    free(root->subdirs);
  }
  free(root);
}

/* debug purpose to print out an rpc_response struct */
void print_response(rpc_response* response) {
  printf("========RPC Response========\n");
  printf("%s: %d\n", HEADER_ERRNO, response->errno_num);
  printf("%s: %d\n", HEADER_RETURN_NUM, response->return_num);
  for (int i = 0; i < response->return_num; i++) {
    printf("%s%d: size=%zu content=%s\n", HEADER_PARAM, i + 1,
           response->return_sizes[i], response->return_vals[i]);
  }
  printf("===========End==========\n");
}