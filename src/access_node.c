#include <netinet/in.h>
#include <pthread.h>
#include <protocol.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <tools.h>
#include <unistd.h>

#define MAX_SLAVES_NUMBER   5
#define MAX_CLIENTS_NUMBER  100

#define SERVER_PORT         1234
#define QUEUE_SIZE          10
#define BUFF_SIZE           256

typedef struct slave_info {
    int socket;
} slave_info;

typedef struct client_info {
    int socket;
} client_info;

int add_client(client_info* clients_list);

int add_slave(slave_info* slaves_list);

void* handle_connection(void* arg);

Request* req_handle(Request* request);

int main(int argc, char** argv) {
    print("Setting up the server.", m_info);
    //c - client, s - slave
    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(struct sockaddr_in));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr.sin_port = htons(SERVER_PORT);

    int s_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (s_socket < 0 || bind(s_socket, (struct sockaddr*) &s_addr, sizeof(struct sockaddr)) < 0) {
        print("Couldn't create a socket.", m_error);
        return 1;
    }
    int on = 1;
    setsockopt(s_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

    if (listen(s_socket, QUEUE_SIZE) < 0) {
        print("Couldn't set queue size.", m_warning);
    }
    print("Setting done. Listening...", m_info);
    while (1) {
        socklen_t socket_size;
        struct sockaddr_in c_addr;
        int c_socket = accept(s_socket, (struct sockaddr*) &c_addr, &socket_size);
        pthread_t thread;
        pthread_create(&thread, NULL, handle_connection, &c_socket);
    }
    close(s_socket);
    return 0;
}

void* handle_connection(void* arg) {
    print("Handling a new request.", m_info);
    int c_socket = *((int*) arg);

    while (1) {
        Request* request = req_create();
        if (!req_receive(c_socket, request)) {
            switch (request->header.req_type) {
                case req_cnt: {
                    RData_Connect* data = req_decode(request);
                    if (data->conn_type == conn_slave) {
                        print("SLAVE", m_info);
                        print(data->name, m_info);
                        print(request->header.key, m_info);
                    }
                    else if (data->conn_type == conn_client) {
                        print("CLIENT", m_info);
                    }
                    free(data);
                    break;
                }
                case req_snd: {
                    RData_File* data = req_decode(request);
                    char buff[256];
                    sprintf(buff, "%u, %lu", data->file_type, data->size);
                    print(buff, m_info);
                    print(data->data, m_info);
                    free(data);
                    break;
                }
                case req_rcv: {

                    break;
                }
                default:
                    break;
            }
            req_free(request);
        }
        else {
            print("Connection with client lost.", m_warning);
            req_free(request);
            break;
        }
    }
    print("Request handled.", m_info);
	close(c_socket);
    return NULL;
}

Request* req_handle(Request* request) {
    switch (request->header.req_type) {
        case req_cnt: {
            RData_Connect* data = req_decode(request);
            if (data->conn_type == conn_slave) {
                print("A new slave node connected, adding to the list...", m_info);
                print(data->name, m_info);
                print(request->header.key, m_info);
            }
            else if (data->conn_type == conn_client) {
                print("A new client connected.", m_info);
            }
            free(data);
            break;
        }
        case req_snd: {
            RData_File* data = req_decode(request);
            char buff[256];
            sprintf(buff, "%u, %lu", data->file_type, data->size);
            print(buff, m_info);
            print(data->data, m_info);
            free(data);
            break;
        }
        case req_rcv: {

            break;
        }
        default:
            break;
    }
}
