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

typedef struct client_info {
    int socket;
} client_info;

typedef struct slave_info {
    int socket;
    int busy;
    int request_id;
    client_info* client;
} slave_info;

//tables with informations about connected slaves and users
slave_info slaves_list[MAX_SLAVES_NUMBER];
client_info clients_list[MAX_CLIENTS_NUMBER];

//mutexes used for accesing slaves and client lists by threads
pthread_mutex_t clients_mutex;
pthread_mutex_t slaves_mutex;

//used for adding client or slave to server's base
client_info* add_client(int c_socket);
slave_info* add_slave(int c_socket);

//functions for handling send requests from clients and slaves
void client_support();
void slave_support();

//res_id is an id for response type
//@TODO add response types in protocol
void send_response(int socket, int res_id);

void* handle_connection(void* arg);

Request* req_handle(Request* request);

int main(int argc, char** argv) {
    print("Setting up the server.", m_info);

    //initializing slaves_list and clients_list
    memset(&slaves_list, 0, MAX_SLAVES_NUMBER*sizeof(slave_info));
    memset(&clients_list, 0, MAX_CLIENTS_NUMBER*sizeof(client_info));
    //initializing mutexes for accessing clients and slaves list
    pthread_mutex_init(&slaves_mutex, NULL);
    pthread_mutex_init(&clients_mutex, NULL);

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
    Request* request = req_create();
    if (!req_receive(c_socket, request) && request->header.req_type == req_cnt) {
        RData_Connect* data = req_decode(request);
        print("Connection request.", m_info);
        connType type = data->conn_type;
        free(data);
        if (type == conn_slave) {
            slave_info* s = add_slave(c_socket);
            if(s != NULL)
                slave_support(s);
            else
                send_response(c_socket, 1); //Response type: no more place for slaves
        }
        else if (type == conn_client) {
            client_info* c = add_client(c_socket);
            if(c != NULL)
                client_support(c);
            else
                send_response(c_socket, 1); //Response type: no more place for clients
        }
        else
            send_response(c_socket, 1); //Response type: wrong conn_type parameter value
    }
    else
        print("Unauthorized device send fail.", m_warning);
        send_response(c_socket, 1); //Response type: unauthorized device send fail
    return NULL;
}

client_info* add_client(int c_socket){ //@TODO authorization
    int i;
    for(i=0; i <= MAX_CLIENTS_NUMBER ; i++) {
        client_info* c = &clients_list[i];
        pthread_mutex_lock(&clients_mutex);
        if(c->socket == 0) {
            c->socket = c_socket;
            pthread_mutex_unlock(&clients_mutex);
            print("New client at socket:", m_info);
            printf("%d\n", c->socket);
            return c;
        }
        else
            pthread_mutex_unlock(&clients_mutex);
    }
    print("NO MORE PLACE FOR CLIENTS", m_warning);
    return NULL;
}

slave_info* add_slave(int c_socket){ //@TODO authorization
  int i;
  for(i = 0; i <= MAX_SLAVES_NUMBER ; i++) {
      slave_info* s = &slaves_list[i];
      pthread_mutex_lock(&slaves_mutex);
      if(s->socket == 0) {
          s->socket = c_socket;
          s->busy = 0;
          pthread_mutex_unlock(&slaves_mutex);
          print("New slave server at socket:", m_info);
          printf("%d\n", s->socket);
          return s;
      }
      else
          pthread_mutex_unlock(&slaves_mutex);
  }
  print("NO MORE PLACE FOR SLAVES", m_warning);
  return NULL;
}

void client_support(client_info* c) {
  while (1) {
      Request* request = req_create();
      if (!req_receive(c->socket, request)) {
          switch (request->header.req_type) {
              case req_snd: {
                  RData_File* data = req_decode(request);
                  print("Send request.",m_info);
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
  memset(c, 0, sizeof(client_info));
  close(c->socket);
}

void slave_support(slave_info* s) {
  while (1) {
      Request* request = req_create();
      if (!req_receive(s->socket, request)) {
          switch (request->header.req_type) {
              case req_snd: {
                  RData_File* data = req_decode(request);
                  print("Send request.",m_info);
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
          print("Connection with slave lost.", m_warning);
          memset(s, 0, sizeof(slave_info));
          req_free(request);
          break;
      }
  }
  print("Request handled.", m_info);
  close(s->socket);
}

void send_response(int sock, int res_id){
  RData_Response data;
  data.res_type = res_id;
  Request* request = req_encode(req_res, &data, "1234567");
  req_send(sock, request);
  req_free(request);
}
//--------------------------------------------------------

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
