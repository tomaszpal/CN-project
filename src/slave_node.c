#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <protocol.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <tools.h>
#include <unistd.h>
#include <Python.h>
#define BUF_SIZE 256

int hand_shake(int s_socket);

int main(int argc, char** argv) {
    if (argv[1] == NULL) {
        print("Missing first parameter (access node address).", m_error);
        return 1;
    }
    if (argv[2] == NULL) {
        print("Missing second parameter (access node port).", m_error);
        return 1;
    }
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
        if (!req_receive(s_socket, request)) {
            print("Connection with the server lost.", m_error);
            req_free(request);
            return 1;
        }
        switch (request->header.req_type) {
            case req_cnt: {
                break;
            }
            case req_snd: {
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

int hand_shake(int s_socket) {
    RData_Connect data;
    data.conn_type = conn_slave;
    strcpy(data.name, "be");
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

    if (request->header.req_type == req_ok) {
        print("Connection with the server established.", m_info);
    }
    else {
        print("Connection refused by the server.", m_error);
        req_free(request);
        return 1;
    }
    req_free(request);
    return 0;
}
