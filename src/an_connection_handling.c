#include "an_connection_handling.h"
#include "an_connection_lists.h"
#include <stdio.h>
#include <stdlib.h>
#include "tools.h"
#include <unistd.h>
#include "queue.h"

extern Client_info clients_list[MAX_SLAVES_NUMBER];
extern Slave_info slaves_list[MAX_CLIENTS_NUMBER];
extern char serverKey[KEY_LENGTH + 1];
extern Tasks_queue tasks_queue;

/* Checks if slave is still connected.                         */
int ping_slave(int socket);

void* handle_connection(void* arg) {
    char buff[BUFF_SIZE];
    int socket = *((int*) arg);
    sprintf(buff, "Handling new connection at socket: %d.", socket);
    print(buff, m_info);
    Request request;
    if (req_receive(socket, &request)) {
        sprintf(buff, "Connection lost at socket: %d.", socket);
        print(buff, m_info);
        close(socket);
        return NULL;
    }
    if (request.header.req_type != req_cnt) {
        print("Unauthorized device.", m_warning);
        if (response_send(socket, res_unauth_dev, -1, serverKey)) {
            sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
            print(buff, m_warning);
        }
        else {
            sprintf(buff, "Response send at socket: %d.", socket);
            print(buff, m_info);
        }
        req_clear(&request);
        close(socket);
        return NULL;
    }
    RData_Connect data;
    if (req_decodeConnect(&request, &data)) {
        print("Couldn't decode the data.", m_warning);
        if (response_send(socket, res_fail, -1, serverKey)) {
            sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
            print(buff, m_warning);
        }
        else {
            sprintf(buff, "Response send at socket: %d.", socket);
            print(buff, m_info);
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
            sprintf(buff, "New slave(id: %d) at socket: %d.", s, socket);
            print(buff, m_info);
            if (response_send(socket, res_ok, -1, serverKey)) {
                sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
                print(buff, m_warning);
                del_slave(s);
                close(socket);
                return NULL;
            }
            sprintf(buff, "Response send at socket: %d.", socket);
            print(buff, m_info);
            slave_support(s);
            del_slave(s);
        }
        else {
            print("Slaves list is full.", m_warning);
            if (response_send(socket, res_full, -1, serverKey)) {
                sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
                print(buff, m_warning);
                close(socket);
                return NULL;
            }
            sprintf(buff, "Response send at socket: %d.", socket);
            print(buff, m_info);
        }
    }
    else if (data.conn_type == conn_client) {
        int c = add_client(socket);
        char buff[BUFF_SIZE];
        if (c >= 0) {
            sprintf(buff, "New client(id: %d) at socket: %d.", c, socket);
            print(buff, m_info);
            if (response_send(socket, res_ok, -1, serverKey)) {
                sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
                print(buff, m_warning);
                del_client(c);
                close(socket);
                return NULL;
            }
            sprintf(buff, "Response send at socket: %d.", socket);
            print(buff, m_info);
            client_support(c);
            del_client(c);
        }
        else {
            print("Clients list is full.", m_warning);
            if (response_send(socket, res_full, -1, serverKey)) {
                sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
                print(buff, m_warning);
                close(socket);
                return NULL;
            }
            sprintf(buff, "Response send at socket: %d.", socket);
            print(buff, m_info);
        }
    }
    else {
        sprintf(buff, "Invalid type of connection at socket: %d.", socket);
        print(buff, m_warning);
        if (response_send(socket, res_fail, -1, serverKey)) {
            sprintf(buff, "Couldn't send response, connection lost at socket: %d.", socket);
            print(buff, m_warning);
            close(socket);
            return NULL;
        }
        sprintf(buff, "Response send at socket: %d.", socket);
        print(buff, m_info);
    }
    sprintf(buff, "Connection handled at socket: %d.", socket);
    print(buff, m_info);
    close(socket);
    return NULL;
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
            int task_id = c->tasks_counter++;
            sprintf(buff, "Send request (id: %d) from client(id: %d).", task_id, id);
            print(buff, m_info);


            RData_File data;
            if (req_decodeFile(&request, &data)) {
                sprintf(buff, "Couldn't decode request data from client(id: %d).", id);
                print(buff, m_warning);
                req_clear(&request);
                if (response_send(c->socket, res_fail, task_id, serverKey)) {
                    sprintf(buff, "Couldn't send response, connection lost with client(id: %d).", id);
                    print(buff, m_warning);
                    break;
                }
                sprintf(buff, "Response send, client(id: %d).", id);
                print(buff, m_info);
                continue;
            }
            req_clear(&request);
            data.id = task_id;

            if (req_encode(&request, req_snd, &data, serverKey)) {
                sprintf(buff, "Couldn't encode request data from client(id: %d).", id);
                print(buff, m_warning);
                fileData_clear(&data);
                if (response_send(c->socket, res_fail, task_id, serverKey)) {
                    sprintf(buff, "Couldn't send response, connection lost with client(id: %d).", id);
                    print(buff, m_warning);
                    break;
                }
                sprintf(buff, "Response send, client(id: %d).", id);
                print(buff, m_info);
                continue;
            }
            //pushing new task into tasks_queue
            if (push(&tasks_queue, task_id, id, &request)) {
                sprintf(buff, "Queue is full, couldn't add client's(id: %d) task(id: %d).", id, task_id);
                print(buff, m_warning);
                req_clear(&request);
                if (response_send(c->socket, res_full, task_id, serverKey)) {
                    sprintf(buff, "Couldn't send response, connection lost with client(id: %d).", id);
                    print(buff, m_warning);
                    break;
                }
                sprintf(buff, "Response send, client(id: %d).", id);
                print(buff, m_info);
                continue;
            }
            req_clear(&request);

            sprintf(buff, "Request from client(id: %d) pushed into tasks queue.", id);
            print(buff, m_info);
            if (response_send(c->socket, res_ok, task_id, serverKey)) {
                sprintf(buff, "Couldn't send response, connection lost with client(id: %d).", id);
                print(buff, m_warning);
                break;
            }
            sprintf(buff, "Response send, client(id: %d).", id);
            print(buff, m_info);
        }
        else if (request.header.req_type == req_rcv) {
            sprintf(buff, "Receive request from client(id: %d).", id);
            print(buff, m_info);
            req_clear(&request);
            int client_id, task_id;
            if (pop(&(c->tasks_done), &task_id, &client_id, &request) ) {
                sprintf(buff, "Client's(id: %d) task queue is empty.", id);
                print(buff, m_warning);
                if (response_send(c->socket, res_empty, -1, serverKey)) {
                    sprintf(buff, "Couldn't send response, connection lost with client(id: %d).", id);
                    print(buff, m_warning);
                    break;
                }
                sprintf(buff, "Response send, client(id: %d).", id);
                print(buff, m_info);
                continue;
            }

            sprintf(buff, "Sending results from task(id: %d) to client(id: %d).", task_id, id);
            print(buff, m_info);
            //send task to client
            if (req_send(c->socket, &request)) {
                req_clear(&request);
                break;
            }
            sprintf(buff, "Send client's(id: %d) task(id: %d) results.", id, task_id);
            print(buff, m_info);
            req_clear(&request);
        }
        else {
            print("Unsupported request type.", m_warning);
            if (response_send(c->socket, res_fail, -1, serverKey)) {
                sprintf(buff, "Couldn't send response, connection lost with client(id: %d).", id);
                print(buff, m_warning);
                break;
            }
            sprintf(buff, "Response send, client(id: %d).", id);
            print(buff, m_info);
            continue;
            req_clear(&request);
        }
    }
    sprintf(buff, "Connection with client(id: %d) lost.", id);
    print(buff, m_warning);
}

void slave_support(int id) {
    char buff[BUFF_SIZE];
    Slave_info* s = &slaves_list[id];
    while (1) {
        Request request;
        int client_id, task_id;
        int is_connected = ping_slave(s->socket);
        //poll and wait for tasks
        while (is_connected && !s->busy && pop(&tasks_queue, &task_id, &client_id, &request)) {
            is_connected = ping_slave(s->socket);
            sleep(POLL_TIME_PERIOD);
        }

        if (!is_connected) {
            break;
        }

        s->busy = 1;
        s->client_id = client_id;
        s->task_id = task_id;
        //send task to slave

        if (req_send(s->socket, &request)) {
            req_clear(&request);
            break;
        }
        req_clear(&request);

        if (req_receive(s->socket, &request)) {
            break;
        }
        s->busy = 0;

        if (request.header.req_type == req_snd || request.header.req_type == req_res) {
            sprintf(buff, "Send request from slave(id: %d).", id);
            print(buff, m_info);
            RData_File temp;
            req_decodeFile(&request, &temp);
            //pushing done task into tasks_done for appropriate client
            if (push(&(clients_list[s->client_id].tasks_done), s->task_id, s->client_id, &request)) {
                sprintf(buff, "Couldn't push result to client's queue(id: %d).", s->client_id);
                print(buff, m_warning);
            }
            sprintf(buff, "Result pushed to client's queue(id: %d) from slave(id: %d).", s->client_id, id);
            print(buff, m_info);
            req_clear(&request);
        }
        else {
            print("Unsupported request type.", m_warning);
            req_clear(&request);
        }
    }
    sprintf(buff, "Connection with slave(id: %d) lost.", id);
    print(buff, m_info);
    if (s->busy) {
        Request request;
        RData_Response data;
        data.res_type = res_fail;
        data.id = s->task_id;
        req_encode(&request, req_res, &data, serverKey);
        if (push(&(clients_list[s->client_id].tasks_done), s->task_id, s->client_id, &request)) {
            sprintf(buff, "Couldn't push result to client's queue(id: %d).", s->client_id);
            print(buff, m_warning);
        }
    }
}

int ping_slave(int socket) {
    if (response_send(socket, res_ok, -1, serverKey)) {
        return 0;
    }
    Request request;
    if (req_receive(socket, &request)) {
        return 0;
    }
    if (request.header.req_type != req_res) {
        req_clear(&request);
        return 0;
    }
    return 1;
}
