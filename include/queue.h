#ifndef _QUEUE_
#define _QUEUE_

#include "protocol.h"
#include <pthread.h>

#define MAX_TASKS_NUMBER 10

typedef struct Task_info{
    int task_id;
    int client_id;
    RData_File resource;
} Task_info;

typedef struct Tasks_queue{
    Task_info queue[MAX_TASKS_NUMBER];
    pthread_mutex_t mutex;
    int first;
    int last;
    int counter;
} Tasks_queue;

void init_queue(Tasks_queue* req);
int is_empty(Tasks_queue* queue);
int is_full(Tasks_queue* queue);
int push(Tasks_queue* queue, int task_id, int client_id, RData_File* resource);
int pop(Tasks_queue* queue, int* task_id, int* client_id, RData_File* resource);

#endif //_QUEUE_
