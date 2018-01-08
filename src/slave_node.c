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

#define BUF_SIZE 256


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
        return 1;
    }
    //4096	87380	6291456


    RData_Connect data;
    data.conn_type = conn_slave;
    sprintf(data.name, "TEST");

    Request* request = req_encode(req_cnt, &data, "1234567");

    req_send(s_socket, request);
    req_free(request);

    RData_File script;
    script.file_type = py3_script;
    script.size = 120;
    script.data = malloc(script.size);
    sprintf(script.data, "Ala ma kota, kot ma Ale!\nGEJ!!");
    request = req_encode(req_snd, &script, "1234567");
    req_send(s_socket, request);
    req_free(request);
    free(script.data);
    // while (1) {
    //     if (!req_receive(s_socket, &request)) {
    //         switch (request.header.req_type) {
    //             case req_cnt: {
    //                 RData_Connect data = *((RData_Connect*)request.data);
    //                 print(data.name ,m_info);
    //             }
    //             default:
    //                 break;
    //         }
    //     } else {
    //         print("Connection with client lost.", m_warning);
    //         break;
    //     }
    // }
    close(s_socket);
    return 0;
}
