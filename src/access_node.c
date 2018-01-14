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

#define MAX_SLAVES_NUMBER   5
#define MAX_CLIENTS_NUMBER  100

#define SERVER_PORT         1234
#define QUEUE_SIZE          10
#define BUFF_SIZE           256

const char serverKey[KEY_LENGTH + 1] = "12345678";

typedef struct Client_info {
    int socket;
    int tasks_counter;
    Tasks_queue tasks_done;
} Client_info;

typedef struct Slave_info {
    int socket;
    int busy;
    int client_id;
    int task_id;
} Slave_info;

Tasks_queue tasks_queue;

//tables with informations about connected slaves and users
Slave_info slaves_list[MAX_SLAVES_NUMBER];
Client_info clients_list[MAX_CLIENTS_NUMBER];

//mutexes used for accesing slaves and client lists by threads
pthread_mutex_t clients_mutex;
pthread_mutex_t slaves_mutex;

//used for adding client or slave to server's base
int add_client(int socket);
int add_slave(int socket);

//used for deleting client or slave from server's base
void del_client(int id);
void del_slave(int id);

//functions for handling send requests from clients and slaves
void client_support(int id);
void slave_support(int id);

void* handle_connection(void* arg);

Request* req_handle(Request* request);

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
    push(&tasks_queue, 0, 0, &qwe);

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

void* handle_connection(void* arg) {
    print("Handling a new connection.", m_info);
    int socket = *((int*) arg);
    Request request;
    if (req_receive(socket, &request)) {
        print("Connection lost.", m_warning);
        close(socket);
        return NULL;
    }
    if (request.header.req_type != req_cnt) {
        print("Unauthorized device.", m_warning);
        req_clear(&request);
        close(socket);
        return NULL;
    }
    RData_Connect data;
    if (req_decodeConnect(&request, &data)) {
        print("Couldn't decode the data.", m_warning);
        if (response_send(socket, res_fail, serverKey)) {
            print("Couldn't send response, connection lost.", m_warning);
        }
        req_clear(&request);
        close(socket);
        return NULL;
    }
    req_clear(&request);

    if (data.conn_type == conn_slave) {
        int s = add_slave(socket);
        if (s >= 0) {
            char buff[BUFF_SIZE];
            sprintf(buff, "New slave at socket: %d.", socket);
            print(buff, m_info);
            if (response_send(socket, res_ok, serverKey)) {
                print("Couldn't send response, connection lost.", m_warning);
                del_slave(s);
                close(socket);
                return NULL;
            }
            slave_support(s);
            del_slave(s);
        }
        else {
            print("Slaves list is full.", m_warning);
            if (response_send(socket, res_full, serverKey)) {
                print("Couldn't send response, connection lost.", m_warning);
                close(socket);
                return NULL;
            }
        }
    }
    else if (data.conn_type == conn_client) {
        int c = add_client(socket);
        char buff[BUFF_SIZE];
        sprintf(buff, "New client at socket: %d.", socket);
        print(buff, m_info);
        if (c >= 0) {
            client_support(c);
            del_client(c);
        }
        else {
            print("Clients list is full.", m_warning);
            if (response_send(socket, res_full, serverKey)) {
                print("Couldn't send response, connection lost.", m_warning);
                close(socket);
                return NULL;
            }
        }
    }
    else {
        print("Invalid type of connection.", m_warning);
        if (response_send(socket, res_fail, serverKey)) {
            print("Couldn't send response, connection lost.", m_warning);
            close(socket);
            return NULL;
        }
    }
    print("Connection handled.", m_info);
    close(socket);
    return NULL;
}

int add_client(int socket){ //@TODO authorization
    int i;
    for (i = 0; i <= MAX_CLIENTS_NUMBER ; i++) {
        Client_info* c = &clients_list[i];
        pthread_mutex_lock(&clients_mutex);
        if (c->socket == 0) {
            c->socket = socket;
            c->tasks_counter = 0;
            pthread_mutex_unlock(&clients_mutex);
            return i;
        }
        else {
            pthread_mutex_unlock(&clients_mutex);
        }
    }
    return -1;
}

int add_slave(int socket){ //@TODO authorization
    int i;
    for (i = 0; i <= MAX_SLAVES_NUMBER ; i++) {
        Slave_info* s = &slaves_list[i];
        pthread_mutex_lock(&slaves_mutex);
        if (s->socket == 0) {
            s->socket = socket;
            s->busy = 0;
            pthread_mutex_unlock(&slaves_mutex);
            return i;
        }
        else {
            pthread_mutex_unlock(&slaves_mutex);
        }
    }
    return -1;
}

void del_client(int id){
    Client_info* c = &clients_list[id];
    memset(c, 0, sizeof(Client_info));
}

void del_slave(int id){
    Slave_info* s = &slaves_list[id];
    memset(s, 0, sizeof(Slave_info));
}

void client_support(int id) {
    char buff[BUFF_SIZE];
    Client_info* c = &clients_list[id];
    while (1) {
        Request request;
        if (req_receive(c->socket, &request)) {
            break;
        }
        if (request.header.req_type == req_snd) {
            RData_File data;
            if (req_decodeFile(&request, &data)) {
                sprintf(buff, "Couldn't decode request data from client(id: %d).", id);
                print(buff, m_warning);
                req_clear(&request);
                continue;
            }
            req_clear(&request);

            sprintf(buff, "Send request from client(id: %d).", id);
            print(buff, m_info);
            //pushing new task into tasks_queue
            if (push(&tasks_queue, c->tasks_counter++, id, &data)) {
                //@TODO
            }

            fileData_clear(&data);
        }
        else if (request.header.req_type == req_rcv) {
            if (!is_empty(&(c->tasks_done))) {
                int client_id, task_id;
                RData_File result;
                pop(&(c->tasks_done), &client_id, &task_id, &result);
                Request req_res;
                req_encode(&req_res, req_snd, &result, serverKey);
                free(result.data);
                //send task to client
                if (req_send(c->socket, &req_res)) {
                    break;
                }
                req_clear(&req_res);
            }
        }
        else {
            print("Unsupported request type.", m_warning);
            req_clear(&request);
        }
    }
    sprintf(buff, "Connection with client(id: %d) lost.", id);
    print(buff, m_info);
}

void slave_support(int id) {
    char buff[BUFF_SIZE];
    Slave_info* s = &slaves_list[id];
    while (1) {
        Request request;
        int client_id, task_id;
        RData_File task;
        //poll and wait for tasks
        while (!s->busy && pop(&tasks_queue, &client_id, &task_id, &task));
        s->busy = 1;
        s->client_id = client_id;
        s->task_id = task_id;
        //send task to slave
        req_encode(&request, req_snd, &task, serverKey);
        free(task.data);

        if (req_send(s->socket, &request)) {
            req_clear(&request);
            break;
        }
        req_clear(&request);

        if (req_receive(s->socket, &request)) {
            break;
        }
        s->busy = 0;

        if (request.header.req_type == req_snd) {
            RData_File data;
            if (req_decodeFile(&request, &data)) {
                sprintf(buff, "Couldn't decode request data from slave(id: %d).", id);
                print(buff, m_warning);
                req_clear(&request);
                continue;
            }
            req_clear(&request);

            sprintf(buff, "Send request from slave(id: %d).", id);
            print(buff, m_info);
            //pushing done task into tasks_done for appropriate client
            if (push(&(clients_list[s->client_id].tasks_done), s->task_id, s->client_id, &data)) {
                sprintf(buff, "Couldn't push result to client's queue(id: %d).", s->client_id);
                print(buff, m_warning);
            }
            fileData_clear(&data);
        }
        else {
            print("Unsupported request type.", m_warning);
            req_clear(&request);
        }
    }
    sprintf(buff, "Connection with slave(id: %d) lost.", id);
    print(buff, m_info);
}
