#include "an_connection_lists.h"
#include <string.h>

//tables with informations about connected slaves and users
Slave_info slaves_list[MAX_SLAVES_NUMBER];
Client_info clients_list[MAX_CLIENTS_NUMBER];

//mutexes used for accesing slaves and client lists by threads
pthread_mutex_t clients_mutex;
pthread_mutex_t slaves_mutex;


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
