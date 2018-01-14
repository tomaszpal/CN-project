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

const char* work_dir;

typedef enum CleanType {
    clean_soft,
    clean_hard
} CleanType;

int clean(CleanType type);
int hand_shake(int s_socket);
int setup();
int do_work(RData_File* result, fileType type, const char* data, unsigned long size);

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
    const char* server_name = argv[1];
    const char* server_port = argv[2];
    work_dir = argv[3];

    setup();
    //c - client, s - slave
    struct hostent* server_ent = gethostbyname(server_name);
    if (!server_ent) {
        print("Can't get the server's IP address.", m_error);
        return 1;
    }

    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(struct sockaddr));
    s_addr.sin_family = AF_INET;
    memcpy(&s_addr.sin_addr.s_addr, server_ent->h_addr, server_ent->h_length);
    s_addr.sin_port = htons(atoi(server_port));

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

    if (hand_shake(s_socket)) {
        print("Handshake with server failed.", m_error);
        close(s_socket);
        return 1;
    }

    print("Connected to the server.", m_info);
    while (1) {
        Request request;
        if (req_receive(s_socket, &request)) {
            print("Connection with the server lost.", m_error);
            break;
        }
        if (request.header.req_type == req_snd) {
            RData_File data;
            if (req_decodeFile(&request, &data)) {
                req_clear(&request);
                print("Couldn't decode request.", m_warning);
                if (response_send(s_socket, res_fail, "1234")) {
                    print("Couldn't send response, connection lost.", m_warning);
                    break;
                }
                continue;
            }
            req_clear(&request);

            RData_File result;
            if (do_work(&result, data.file_type, data.data, data.size)) {
                if (response_send(s_socket, res_fail, "1234")) {
                    print("Couldn't send response, connection lost.", m_warning);
                    break;
                }
                continue;
            }
            clean(clean_soft);
            fileData_clear(&data);

            if (req_encode(&request, req_snd, &result, "1234")) {
                fileData_clear(&result);
                print("Couldn't encode result.", m_warning);
                if (response_send(s_socket, res_fail, "1234")) {
                    print("Couldn't send response, connection lost.", m_warning);
                    break;
                }
                continue;
            }
            fileData_clear(&result);

            if (req_send(s_socket, &request)) {
                print("Connection with the server lost.", m_error);
                req_clear(&request);
                return 1;
            }
            req_clear(&request);
            print("Result send to server", m_info);
        }
        else {
            print("Unsupported request type.", m_warning);
            req_clear(&request);
        }
    }
    close(s_socket);
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
    result->data = malloc(result->size);
    if (result->data == NULL) {
        return 1;
    }
    lseek(fd, 0, SEEK_SET);
    read(fd, result->data, result->size - 1);
    result->data[result->size - 1] = 0;
    close(fd);
    return 0;
}

int hand_shake(int s_socket) {
    RData_Connect data;
    data.conn_type = conn_slave;
    Request request;
    req_encode(&request, req_cnt, &data, "1234");

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
