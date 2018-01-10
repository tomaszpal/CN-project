#ifndef _PROTOCOL_
#define _PROTOCOL_

/* Defines request types.                                       */
typedef enum reqType {
    req_cnt,
    req_snd,
    req_rcv,
    req_res
} reqType;

/* Defines connection types.                                    */
typedef enum connType {
    conn_client,
    conn_slave
} connType;

/* Defines file type.                                          */
typedef enum fileType {
    file_py2_script,
    file_py3_script,
    file_data_file
} fileType;

/*Defines server response types                                */
typedef enum resType {
    res_ok,
    res_fail,
    res_full,
    res_unauth_dev
    //add more
} resType;

/* Defines request header.                                      */
typedef struct Header {
    reqType req_type;
    unsigned long size;
    char key[8];
} Header;

/* Defines request.                                             */
typedef struct Request {
    Header header;
    void* data;
} Request;

/* Defines login request data.                                   */
typedef struct RData_Connect {
    connType conn_type;
    char name[16];
    char password[16];
} RData_Connect;

/* Defines file sending request data.                            */
typedef struct RData_File {
    fileType file_type;
    unsigned long size;
    char* data;
} RData_File;

/* Defines server response                                       */
typedef struct RData_Response {
    resType res_type;
} RData_Response;
#endif //_PROTOCOL_
