#ifndef _AN_CONNECTION_LISTS_
#define _AN_CONNECTION_LISTS_

#define MAX_SLAVES_NUMBER   5
#define MAX_CLIENTS_NUMBER  100

#include "queue.h"

/* Defines the informations about client.                       */
typedef struct Client_info {
    int session_id;
    int socket;
    int tasks_counter;
    Tasks_queue tasks_done;
} Client_info;

/* Defines the informations about slave.                        */
typedef struct Slave_info {
    int socket;
    int busy;
    int client_id;
    int task_id;
} Slave_info;

/* Adds client to the clients list.                             */
int add_client(int socket);

/* Adds slave to the slaves list.                               */
int add_slave(int socket);

/* Deletes client to the clients list.                          */
void del_client(int id);

/* Deletes slave to the slaves list.                            */
void del_slave(int id);

#endif //_AN_CONNECTION_LISTS_
