#include <fcntl.h>
#include "protocol.h"
#include "slave_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "tools.h"
#include <unistd.h>

const char* work_dir;

int clean(CleanType type) {
    char buff[BUFF_SIZE];
    switch (type) {
        case clean_soft: {
            sprintf(buff, "rm -rf %s/*", work_dir);
        } break;
        case clean_hard: {
            sprintf(buff, "rm -rf %s", work_dir);
        } break;
        default: {
            return 1;
        }
    }
    return system(buff);
}

int do_work(RData_File* result, fileType type, const char* data, unsigned long size) {
    memset(result, 0, sizeof(RData_File));
    char buff[BUFF_SIZE];

    sprintf(buff, "%s/temp.py", work_dir);
    int fd = open(buff, O_WRONLY | O_CREAT, 0700);
    if (fd < 0) {
        print("Couldn't create temporary python file", m_warning);
        return 1;
    }
    write(fd, data, size);
    close(fd);

    if (type == file_py2_script) {
        sprintf(buff, "python2 %s/temp.py >> %s/result 2>&1", work_dir, work_dir);
    }
    else if (type == file_py3_script) {
        sprintf(buff, "python3 %s/temp.py >> %s/result 2>&1", work_dir, work_dir);
    }

    if (system(buff)) {
        print("Running script failed.", m_warning);
    }
    else {
        print("Script executed properly.", m_info);
    }

    sprintf(buff, "%s/result", work_dir);
    fd = open(buff, O_RDONLY);
    if (fd < 0) {
        print("Couldn't open result file.", m_warning);
        return 1;
    }

    result->file_type = file_data_file;
    result->size = lseek(fd, 0, SEEK_END);
    result->data = malloc(result->size + 1);
    if (result->data == NULL) {
        return 1;
    }
    lseek(fd, 0, SEEK_SET);
    read(fd, result->data, result->size);
    result->data[result->size] = 0;
    close(fd);
    return 0;
}

int handshake(int s_socket, const char key[KEY_LENGTH + 1]) {
    RData_Connect data;
    data.conn_type = conn_slave;
    Request request;
    req_encode(&request, req_cnt, &data, key);

    if (req_send(s_socket, &request)) {
        print("Connection with the server lost.", m_error);
        req_clear(&request);
        return 1;
    }
    req_clear(&request);

    if (req_receive(s_socket, &request)) {
        print("Connection with the server lost.", m_error);
        req_clear(&request);
        return 1;
    }

    if (request.header.req_type == req_res) {
        RData_Response response;
        req_decodeResponse(&request, &response);
        req_clear(&request);
        if (response.res_type == res_ok) {
            print("Connection with the server established.", m_info);
        }
        else {
            print("Connection refused by the server.", m_error);
            return 1;
        }
    }
    else {
        print("Unknown or wrong response from the server.", m_error);
        return 1;
    }
    return 0;
}

int setup() {
    if (system("python2 --version > /dev/null 2>&1")) {
        print("Python2 interpreter not found!", m_warning);
        return 1;
    }
    if (system("python3 --version > /dev/null 2>&1")) {
        print("Python3 interpreter not found!", m_warning);
        return 1;
    }

    struct stat st = {0};
    if (stat(work_dir, &st) == -1) {
        if (mkdir(work_dir, 0700)) {
            print("Couldn't create work directory", m_error);
            return 1;
        }
    }
    return 0;
}
