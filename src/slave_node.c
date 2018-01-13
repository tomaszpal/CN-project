#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <protocol.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <tools.h>
#include <unistd.h>

#define BUFF_SIZE 256

int clean(const char* work_dir, int hard);
int hand_shake(int s_socket);
int setup(const char* work_dir);
RData_File* do_work(const char* work_dir, fileType type, char* data, unsigned long size);

int main(int argc, char** argv) {
    if (argv[1] == NULL) {
        print("Missing first parameter (access node address).", m_error);
        return 1;
    }
    if (argv[2] == NULL) {
        print("Missing second parameter (access node port).", m_error);
        return 1;
    }
    if (argv[3] == NULL) {
        print("Missing third parameter (work directory).", m_error);
        return 1;
    }
    setup(argv[3]);
    //c - client, s - slave
    struct hostent* server_ent = gethostbyname(argv[1]);
    if (!server_ent) {
        print("Can't get the server's IP address.", m_error);
        return 1;
    }

    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(struct sockaddr));
    s_addr.sin_family = AF_INET;
    memcpy(&s_addr.sin_addr.s_addr, server_ent->h_addr, server_ent->h_length);
    s_addr.sin_port = htons(atoi(argv[2]));

    int s_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (s_socket < 0) {
        print("Couldn't create a socket.", m_error);
        return 1;
    }

    if (connect(s_socket, (struct sockaddr*) &s_addr, sizeof(struct sockaddr)) < 0) {
        print("Couldn't connect to the server.", m_error);
        close(s_socket);
        return 1;
    }

    if (!hand_shake(s_socket)) {
        print("Connected to the server.", m_info);
    }
    else {
        print("Handshake with server failed.", m_error);
        close(s_socket);
        return 1;
    }

    while (1) {
        Request* request = req_create();
        if (req_receive(s_socket, request)) {
            print("Connection with the server lost.", m_error);
            req_free(request);
            return 1;
        }
        switch (request->header.req_type) {
            case req_snd: {
                RData_File* data = req_decode(request);
                RData_File* result = do_work(argv[3], data->file_type, data->data, data->size);
                clean(argv[3], 0);
                print(result->data, m_info);
                free(data->data);
                free(data);
                Request* request_response;
                if (result == NULL) {
                    print("Couldn't run python script", m_warning);
                    RData_Response response;
                    response.res_type = res_fail;
                    request_response = req_encode(req_res, &response, "1234");
                }
                else {
                    print("Script run correctly", m_info);
                    request_response = req_encode(req_snd, result, "1234");
                    free(result->data);
                    free(result);
                }

                if (req_send(s_socket, request_response)) {
                    print("Connection with the server lost.", m_error);
                    req_free(request_response);
                    req_free(request);
                    return 1;
                }
                print("Output send to server", m_info);
                req_free(request_response);
                break;
            }
            default:
                break;
        }
        req_free(request);
    }
    close(s_socket);
    return 0;
}

int setup(const char* work_dir) {
    if (system("python2 --version > /dev/null 2>&1")) {
        print("Python2 interpreter not found!", m_warning);
        return 0;
    }
    if (system("python3 --version > /dev/null 2>&1")) {
        print("Python3 interpreter not found!", m_warning);
        return 0;
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

int clean(const char* work_dir, int hard) {
    char buff[BUFF_SIZE];
    if (hard) {
        sprintf(buff, "rm -rf %s", work_dir);
    }
    else {
        sprintf(buff, "rm -rf %s/*", work_dir);
    }
    if (system(buff)) {
        print("Couldn't clear work directory", m_warning);
    }
    return 0;
}

RData_File* do_work(const char* work_dir, fileType type, char* data, unsigned long size) {
    char buff[BUFF_SIZE];
    sprintf(buff, "%s/temp.py", work_dir);

    int fd = open(buff, O_WRONLY | O_CREAT, 0700);
    if (fd < 0) {
        print("Couldn't create temporary python file", m_warning);
        return NULL;
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
        print("Running script failed", m_warning);
    }
    sprintf(buff, "%s/result", work_dir);
    RData_File* result = malloc(sizeof(RData_File));
    result->file_type = file_data_file;
    fd = open(buff, O_RDONLY);
    if (fd < 0) {
        print("Couldn't open resutl file", m_warning);
        free(result);
        return NULL;
    }
    result->size = lseek(fd, 0, SEEK_END);
    result->data = malloc(result->size);
    lseek(fd, 0, SEEK_SET);
    int x = read(fd, result->data, result->size - 1);
    result->data[result->size - 1] = 0;
    close(fd);
    return result;
}

int hand_shake(int s_socket) {
    RData_Connect data;
    data.conn_type = conn_slave;
    Request* request = req_encode(req_cnt, &data, "1234");

    if (req_send(s_socket, request)) {
        print("Connection with the server lost.", m_error);
        req_free(request);
        return 1;
    }
    req_free(request);

    request = req_create();
    if (req_receive(s_socket, request)) {
        print("Connection with the server lost.", m_error);
        req_free(request);
        return 1;
    }

    if (request->header.req_type == req_res) {
        RData_Response* response = req_decode(request);
        if (response->res_type == res_ok) {
            print("Connection with the server established.", m_info);
            free(response);
        }
        else {
            print("Connection refused by the server.", m_error);
            req_free(request);
            free(response);
            return 1;
        }
    }
    else {
        print("Unknown or wrong response from the server.", m_error);
        req_free(request);
        return 1;
    }
    req_free(request);
    return 0;
}
