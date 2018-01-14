#include "an_connection_handling.h"
#include "an_connection_lists.h"
#include <netinet/in.h>
#include <pthread.h>
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "tools.h"
#include <unistd.h>
#include "queue.h"

#define SERVER_PORT         1234
#define QUEUE_SIZE          10

const char serverKey[KEY_LENGTH + 1] = "12345678";

//queue with tasks to execute
Tasks_queue tasks_queue;

//tables with informations about connected slaves and users
extern Slave_info slaves_list[MAX_SLAVES_NUMBER];
extern Client_info clients_list[MAX_CLIENTS_NUMBER];

//mutexes used for accesing slaves and client lists by threads
extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t slaves_mutex;

int main(int argc, char** argv) {
    print("Setting up the server.", m_info);
    //initializing tasks_queue
    init_queue(&tasks_queue);
    //initializing slaves_list and clients_list
    memset(&slaves_list, 0, MAX_SLAVES_NUMBER * sizeof(Slave_info));
    memset(&clients_list, 0, MAX_CLIENTS_NUMBER * sizeof(Client_info));
    //initializing mutexes for accessing clients and slaves list
    pthread_mutex_init(&slaves_mutex, NULL);
    pthread_mutex_init(&clients_mutex, NULL);

    RData_File qwe;
    qwe.file_type = file_py3_script;
    char script[] = "print('FooBAR')\n";
    qwe.size = sizeof(script);
    qwe.data = script;
    Request req;
    req_encode(&req, req_snd, &qwe, serverKey);
    push(&tasks_queue, 0, 0, &req);

    //setup server
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
        struct sockaddr_in addr;
        int socket = accept(s_socket, (struct sockaddr*) &addr, &socket_size);
        pthread_t thread;
        pthread_create(&thread, NULL, handle_connection, &socket);
    }
    close(s_socket);
    return 0;
}
