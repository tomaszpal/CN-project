#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tools.h>
#include <unistd.h>

void print(char* message, mType type) {
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

int req_send(int socket, Request* request) {
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

int req_receive(int socket, Request* request) {
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

    nbytes = 0;
    ptr = (char *) request->data;
    while (nbytes < request->header.size) {
        int nread = read(socket, ptr + nbytes, request->header.size - nbytes);
        if (nread == 0) {
            return 1;
        }
        nbytes += nread;
    }
    return 0;
}

Request* req_encode(reqType type, void* data, char key[8]) {
    Request* request = req_create();
    request->header.req_type = type;
    strcpy(request->header.key, key);
    switch (type) {
        case req_res:
        case req_cnt: {
            request->header.size = sizeof(RData_Connect);
            request->data = malloc(request->header.size);
            memcpy(request->data, data, request->header.size);
            break;
        }
        case req_snd: {
            RData_File* tmp = data;
            request->header.size = sizeof(RData_File) + tmp->size;
            request->data = malloc(request->header.size);
            char* ptr = request->data;
            memcpy(ptr, data, sizeof(RData_File));
            memcpy(ptr + sizeof(RData_File), tmp->data, tmp->size);
            break;
        }
        default:
            req_free(request);
            return NULL;
    }
    return request;
}

void* req_decode(Request* request) {
    void* res = NULL;
    switch (request->header.req_type) {
        case req_cnt: {
            RData_Connect* data = malloc(sizeof(RData_Connect));
            memcpy(data, request->data, request->header.size);
            res = data;
            break;
        }
        case req_snd: {
            RData_File* data = malloc(sizeof(RData_File));
            memcpy(data, request->data, sizeof(RData_File));
            data->data = malloc(data->size);
            char* ptr = request->data;
            memcpy(data->data, ptr + sizeof(RData_File), data->size);
            res = data;
            break;
        }
        default:
            break;
    }
    return res;
}

Request* req_create() {
    Request* request = malloc(sizeof(Request));
    if (request) {
        memset(request, 0, sizeof(Request));
    }
    return request;
}

void req_free(Request* request) {
    free(request->data);
    free(request);
}
