#ifndef _PROTOCOL_
#define _PROTOCOL_

/* Defines request types.                                       */
typedef enum reqType {
    req_cnt,
    req_snd,
    req_rfh,
    req_rcv,
    req_fail,
    req_ok
} reqType;

/* Defines connection types.                                    */
typedef enum connType {
    conn_client,
    conn_slave
} connType;

/* Defines file type.                                          */
typedef enum fileType {
    py2_script,
    py3_script,
    data_file
} fileType;

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

#endif //_PROTOCOL_
