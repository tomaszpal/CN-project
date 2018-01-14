#ifndef _TOOLS_
#define _TOOLS_

#include "protocol.h"

/* Defines colors of output.                                    */
#define COL_RED   "\x1B[31m"
#define COL_ORG   "\x1B[38;5;202m"
#define COL_GRN   "\x1B[32m"
#define COL_WHT   "\x1B[0m"

/* Defines message types.                                       */
typedef enum mType{
    m_error,
    m_warning,
    m_info
} mType;

/* Prints different types of messages on output.               */
void print(const char* message, mType type);

/* Sends a request to a given socket.                          */
int req_send(int socket, const Request* request);

/* Sends a response request to a given socket.                 */
int response_send(int socket, resType res_id, const char key[8]);

/* Receives a request from a given socket.                     */
int req_receive(int socket, Request* request);

/* Encodes a request data to a request.                        */
int req_encode(Request* request, reqType type, const void* data, const char key[KEY_LENGTH + 1]);

/* Decodes a connect data from a given request.                */
int req_decodeConnect(const Request* request, RData_Connect* data);

/* Decodes a file data from a given request.                   */
int req_decodeFile(const Request* request, RData_File* data);

/* Decodes a response data from a given request.               */
int req_decodeResponse(const Request* request, RData_Response* data);

/* Clears a request data from a given request.                 */
void req_clear(Request* request);

/* Clears a file data from file sending request data.          */
void fileData_clear(RData_File* data);

#endif //_TOOLS_
