#include <netdb.h>
#include <netinet/in.h>
#include "protocol.h"
#include "slave_tools.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "tools.h"
#include <unistd.h>


//in this folder all temporary python files and results will be stored
//after sending result to access node they will be removed
extern const char* work_dir;

const char slaveKey[KEY_LENGTH + 1] = "12345678";

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

    //check for python interpreters and create work directory
    setup();

    struct hostent* server_ent = gethostbyname(server_name);
    if (!server_ent) {
        print("Can't get the server's IP address.", m_error);
        return 1;
    }
    //s - server
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

    //tell access node that you are a slave node
    if (handshake(s_socket, slaveKey)) {
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
                if (response_send(s_socket, res_fail, 0, slaveKey)) {
                    print("Couldn't send response, connection lost.", m_warning);
                    break;
                }
                continue;
            }
            req_clear(&request);
            int task_id = data.id;
            RData_File result;
            if (do_work(&result, data.file_type, data.data, data.size)) {
                if (response_send(s_socket, res_fail, task_id, slaveKey)) {
                    print("Couldn't send response, connection lost.", m_warning);
                    break;
                }
                continue;
            }
            clean(clean_soft);
            fileData_clear(&data);
            result.id = task_id;
            if (req_encode(&request, req_snd, &result, slaveKey)) {
                fileData_clear(&result);
                print("Couldn't encode result.", m_warning);
                if (response_send(s_socket, res_fail, task_id, slaveKey)) {
                    print("Couldn't send response, connection lost.", m_warning);
                    break;
                }
                continue;
            }
            fileData_clear(&result);

            if (req_send(s_socket, &request)) {
                print("Connection with the server lost.", m_error);
                req_clear(&request);
                break;
            }
            req_clear(&request);
            print("Result send to server", m_info);
        }
        else if (request.header.req_type == req_res) {
            RData_Response data;
            req_decodeResponse(&request, &data);
            if (data.res_type == res_ok) {
                if (req_send(s_socket, &request)) {
                    print("Connection with the server lost.", m_error);
                    req_clear(&request);
                    break;
                }
            }
            else {
                print("Unsupported response type.", m_warning);
            }
        }
        else {
            print("Unsupported request type.", m_warning);
            req_clear(&request);
        }
    }
    close(s_socket);
    return 0;
}
