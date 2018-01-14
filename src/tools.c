#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"
#include <unistd.h>

#define member_size(type, member) sizeof(((type *)0)->member)

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
    int size = member_size(Header, req_type) + member_size(Header, size)
               + member_size(Header, key) + request->header.size;

    char* data = malloc(size);
    if (data == NULL) {
        return 1;
    }
    memset(data, 0, size);
    char* ptr = data;
    memcpy(ptr, &(request->header.req_type), member_size(Header, req_type));
    ptr += member_size(Header, req_type);
    memcpy(ptr, &(request->header.size), member_size(Header, size));
    ptr += member_size(Header, size);
    memcpy(ptr, request->header.key, member_size(Header, key));
    ptr += member_size(Header, key);
    memcpy(ptr, request->data, request->header.size);

    int nbytes = 0;
    ptr = data;
    while (nbytes < size) {
        int nwrite = write(socket, ptr + nbytes, size - nbytes);
        if (nwrite == 0) {
            free(data);
            return 1;
        }
        nbytes += nwrite;
    }
    free(data);
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
    int header_size = member_size(Header, req_type) + member_size(Header, size) +
               member_size(Header, key);

    char* data = malloc(header_size);
    memset(data, 0, header_size);
    memset(request, 0, sizeof(Request));

    int nbytes = 0;
    char* ptr = data;
    while (nbytes < header_size) {
        int nread = read(socket, ptr + nbytes, header_size - nbytes);
        if (nread == 0) {
            return 1;
        }
        nbytes += nread;
    }
    memcpy(&(request->header.req_type), ptr, member_size(Header, req_type));
    ptr += member_size(Header, req_type);
    memcpy(&(request->header.size), ptr, member_size(Header, size));
    ptr += member_size(Header, size);
    memcpy(request->header.key, ptr, member_size(Header, key));
    ptr += member_size(Header, key);
    free(data);

    request->data = malloc(request->header.size);
    if (request->header.size != 0 && request->data == NULL) {
        return 2;
    }
    memset(request->data, 0, request->header.size);

    nbytes = 0;
    ptr = (char *)request->data;
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
        case req_res: {
            const RData_Response* response = data;
            int size = member_size(RData_Response, res_type) + member_size(RData_Response, id);

            request->header.size = size;
            request->data = malloc(size);
            if (request->data == NULL) {
                return 1;
            }
            memset(request->data, 0, size);
            char* ptr = (char*)request->data;

            memcpy(ptr, &(response->res_type), member_size(RData_Response, res_type));
            ptr += member_size(RData_Response, res_type);
            memcpy(ptr, &(response->id), member_size(RData_Response, id));

        } break;
        case req_cnt: {
            const RData_Connect* connect = data;
            int size = member_size(RData_Connect, conn_type) + member_size(RData_Connect, login)
                       + member_size(RData_Connect, passwd);

            request->header.size = size;
            request->data = malloc(size);

            if (request->data == NULL) {
                return 1;
           }
           memset(request->data, 0, size);
           char* ptr = (char*)request->data;

           memcpy(ptr, &(connect->conn_type), member_size(RData_Connect, conn_type));
           ptr += member_size(RData_Connect, conn_type);
           memcpy(ptr, &(connect->login), member_size(RData_Connect, login));
           ptr += member_size(RData_Connect, login);
           memcpy(ptr, &(connect->passwd), member_size(RData_Connect, passwd));

        } break;
        case req_snd: {
            const RData_File* file = data;

            int size = member_size(RData_File, id) + member_size(RData_File, file_type)
                       + member_size(RData_File, size) + file->size;

            request->header.size = size;
            request->data = malloc(size);

            if (request->data == NULL) {
                return 1;
           }
           memset(request->data, 0, size);
           char* ptr = (char*)request->data;

           memcpy(ptr, &(file->id), member_size(RData_File, id));
           ptr += member_size(RData_File, id);
           memcpy(ptr, &(file->file_type), member_size(RData_File, file_type));
           ptr += member_size(RData_File, file_type);
           memcpy(ptr, &(file->size), member_size(RData_File, size));
           ptr += member_size(RData_File, size);
           memcpy(ptr, file->data, file->size);
        } break;
        default:
            return 1;
    }
    return 0;
}

int req_decodeConnect(const Request* request, RData_Connect* data) {
    memset(data, 0, sizeof(RData_Connect));

    char* ptr = (char*)request->data;
    memcpy(&(data->conn_type), ptr, member_size(RData_Connect, conn_type));
    ptr += member_size(RData_Connect, conn_type);
    memcpy(&(data->login), ptr, member_size(RData_Connect, login));
    ptr += member_size(RData_Connect, login);
    memcpy(&(data->passwd), ptr, member_size(RData_Connect, passwd));

    return 0;
}

int req_decodeFile(const Request* request, RData_File* data) {
    memset(data, 0, sizeof(RData_File));

    char* ptr = request->data;
    memcpy(&(data->id), ptr, member_size(RData_File, id));
    ptr += member_size(RData_File, id);
    memcpy(&(data->file_type), ptr, member_size(RData_File, file_type));
    ptr += member_size(RData_File, file_type);
    memcpy(&(data->size), ptr, member_size(RData_File, size));
    ptr += member_size(RData_File, size);

    data->data = malloc(data->size);
    if (data->data == NULL) {
        return 1;
    }
    memcpy(data->data, ptr, data->size);

    return 0;
}

int req_decodeResponse(const Request* request, RData_Response* data) {
    memset(data, 0, sizeof(RData_Response));

    char* ptr = (char*)request->data;
    memcpy(&(data->res_type), ptr, member_size(RData_Response, res_type));
    ptr += member_size(RData_Response, res_type);
    memcpy(&(data->id), ptr, member_size(RData_Response, id));
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
