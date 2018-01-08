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
void print(char* message, mType type);

/* Sends a request to a given socket.                          */
int req_send(int socket, Request* request);

/* Receives a request from a given socket.                     */
int req_receive(int socket, Request* request);

/* Encodes a request data to a request.                        */
Request* req_encode(reqType type, void* data, char key[8]);

/* Decodes a request data from a given request.                */
void* req_decode(Request* request);

/* Creates and initializes with zeroes a request.              */
Request* req_create();

/* Frees a request data from a given request.                  */
void req_free(Request* request);

#endif //_TOOLS_
