#ifndef _QUEUE_
#define _QUEUE_

#include "protocol.h"
#include <pthread.h>

/* Defines size of tasks queue.                                 */
#define MAX_TASKS_NUMBER 10

/* Defines informations about a single task.                    */
typedef struct Task_info {
    int task_id;
    int client_id;
    Request resource;
} Task_info;

/* Defines informations about a tasks queue.                    */
typedef struct Tasks_queue {
    Task_info queue[MAX_TASKS_NUMBER];
    pthread_mutex_t mutex;
    int first;
    int last;
    int counter;
} Tasks_queue;

/* Initializes given tasks queue.                               */
void init_queue(Tasks_queue* req);

/* Checks if given tasks queue is empty.                        */
int is_empty(Tasks_queue* queue);

/* Checks if given tasks queue is full.                         */
int is_full(Tasks_queue* queue);

/* Pushes a single task at the end of given queue.              */
int push(Tasks_queue* queue, int task_id, int client_id, Request* resource);

/* Pops a single task from the start of given queue.            */
int pop(Tasks_queue* queue, int* task_id, int* client_id, Request* resource);

#endif //_QUEUE_
