#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"
#include <unistd.h>

#define BUFF_SIZE 256

void print(const char* message, mType type) {
    if (message != NULL) {
        switch (type) {
            case m_error:
                fprintf(stderr, "[" COL_RED "ERROR" COL_WHT "] : %s\n", message);
                break;
            case m_warning:
                fprintf(stdout, "[" COL_ORG "WARNING" COL_WHT "]: %s\n", message);
                break;
            case m_info:
                fprintf(stdout, "[" COL_GRN "INFO" COL_WHT "]: %s\n", message);
                break;
        }
    }
}

int req_send(int socket, const Request* request) {
    int nbytes = 0;
    char* ptr = (char*) &(request->header);
    while (nbytes < sizeof(Header)) {
        int nwrite = write(socket, ptr + nbytes, sizeof(Header) - nbytes);
        if (nwrite == 0) {
            return 1;
        }
        nbytes += nwrite;
    }

    nbytes = 0;
    ptr = (char*) request->data;
    while (nbytes < request->header.size) {
        int nwrite = write(socket, ptr + nbytes, request->header.size - nbytes);
        if (nwrite == 0) {
            return 1;
        }
        nbytes += nwrite;
    }
    return 0;
}

int response_send(int socket, resType res_id, int id, const char key[8]) {
    RData_Response data;
    data.res_type = res_id;
    data.id = id;
    Request request;
    if (req_encode(&request, req_res, &data, key)) {
        return 2;
    }
    if (req_send(socket, &request)) {
        req_clear(&request);
        return 1;
    }
    req_clear(&request);
    return 0;
}

int req_receive(int socket, Request* request) {
    memset(request, 0, sizeof(Request));
    int nbytes = 0;
    char* ptr = (char*) &(request->header);
    while (nbytes < sizeof(Header)) {
        int nread = read(socket, ptr + nbytes, sizeof(Header) - nbytes);
        if (nread == 0) {
            return 1;
        }
        nbytes += nread;
    }
    request->data = malloc(request->header.size);
    if (request->data == NULL) {
        return 2;
    }

    nbytes = 0;
    ptr = (char *) request->data;
    while (nbytes < request->header.size) {
        int nread = read(socket, ptr + nbytes, request->header.size - nbytes);
        if (nread == 0) {
            free(request->data);
            return 1;
        }
        nbytes += nread;
    }
    return 0;
}

int req_encode(Request* request, reqType type, const void* data, const char key[8]) {
    memset(request, 0, sizeof(Request));
    request->header.req_type = type;
    strcpy(request->header.key, key);
    switch (type) {
        case req_res:
        case req_cnt: {
            request->header.size = sizeof(RData_Connect);
            request->data = malloc(request->header.size);
            if (request->data == NULL) {
                return 1;
            }
            memcpy(request->data, data, request->header.size);
            break;
        }
        case req_snd: {
            const RData_File* tmp = data;
            request->header.size = sizeof(RData_File) + tmp->size;
            request->data = malloc(request->header.size);
            if (request->data == NULL) {
                return 1;
            }
            char* ptr = request->data;
            memcpy(ptr, data, sizeof(RData_File));
            memcpy(ptr + sizeof(RData_File), tmp->data, tmp->size);
            break;
        }
        default:
            return 1;
    }
    return 0;
}

int req_decodeConnect(const Request* request, RData_Connect* data) {
    memset(data, 0, sizeof(RData_Connect));
    memcpy(data, request->data, request->header.size);
    return 0;
}

int req_decodeFile(const Request* request, RData_File* data) {
    memset(data, 0, sizeof(RData_File));
    memcpy(data, request->data, sizeof(RData_File));
    data->data = malloc(data->size);
    if (data->data == NULL) {
        return 1;
    }
    char* ptr = request->data;
    memcpy(data->data, ptr + sizeof(RData_File), data->size);
    return 0;
}

int req_decodeResponse(const Request* request, RData_Response* data) {
    memset(data, 0, sizeof(RData_Response));
    memcpy(data, request->data, request->header.size);
    return 0;
}

void req_clear(Request* request) {
    free(request->data);
    memset(request, 0, sizeof(Request));
}

void fileData_clear(RData_File* data) {
    free(data->data);
    memset(data, 0, sizeof(RData_File));
}
