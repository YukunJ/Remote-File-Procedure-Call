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

}

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
rpc_request* deserialize_request(char* buf, size_t buf_size);

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
void pack_pointer(rpc_request* request, int offset, char* buf, size_t buf_size);

/* debug purpose to print out an rpc_request struct */
void print_request(rpc_request* request) {
    printf("========RPC Request========\n");
    printf("%s: %d\n", HEADER_COMMAND, request->command_op);
    printf("%s: %d\n", HEADER_PARAM_NUM, request->param_num);
    for (int i = 0; i < request->param_num; i++) {
        printf("%s%d: size=%zu content=%s\n", HEADER_PARAM, i+1, request->param_sizes[i], request->params[i]);
    }
    printf("===========End==========\n");
}