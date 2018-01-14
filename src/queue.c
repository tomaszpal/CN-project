#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"

void init_queue(Tasks_queue* queue){
    pthread_mutex_init(&(queue->mutex), NULL);
    queue->first = 0;
    queue->last = -1;
    queue->counter = 0;
}

int is_empty(Tasks_queue* queue) {
    pthread_mutex_lock(&(queue->mutex));
    int res = queue->counter;
    pthread_mutex_unlock(&(queue->mutex));
    return res;
}

int is_full(Tasks_queue* queue) {
    pthread_mutex_lock(&(queue->mutex));
    int res = queue->counter == MAX_TASKS_NUMBER;
    pthread_mutex_unlock(&(queue->mutex));
    return res;
}

//return 0 if success, 1 if is already full
//parameters: queue - queue structure pointer
//t - pointer to Task_info structure to be pushed into queue
int push(Tasks_queue* queue, int task_id, int client_id, RData_File* resource) {
    pthread_mutex_lock(&(queue->mutex));
    if (queue->counter != MAX_TASKS_NUMBER) {
        if (queue->last == MAX_TASKS_NUMBER - 1) {
            queue->last = -1;
        }
        Task_info* new_task = &(queue->queue[++(queue->last)]);
        new_task->task_id = task_id;
        new_task->client_id = client_id;
        new_task->resource.file_type = resource->file_type;
        new_task->resource.size = resource->size;
        new_task->resource.data = malloc(new_task->resource.size);
        memcpy(new_task->resource.data, resource->data, new_task->resource.size);
        queue->counter++;
        pthread_mutex_unlock(&(queue->mutex));
        return 0;
    }
    else {
        pthread_mutex_unlock(&(queue->mutex));
        return 1;
    }
}
//return 0 if success, 1 if is already empty
//parameters: queue - queue structure pointer
//t - pointer to Task_info structure to save popped value from queue
int pop(Tasks_queue* queue, int* task_id, int* client_id, RData_File* resource) {
     pthread_mutex_lock(&(queue->mutex));
     if (queue->counter != 0) {
         Task_info* task = &(queue->queue[queue->first++]);
         *task_id = task->task_id;
         *client_id = task->client_id;
         resource->file_type = task->resource.file_type;
         resource->size = task->resource.size;
         resource->data = malloc(resource->size);
         memcpy(resource->data, task->resource.data, resource->size);
         free(task->resource.data);
         if (queue->first == MAX_TASKS_NUMBER) {
            queue->first = 0;
         }
         queue->counter--;
         pthread_mutex_unlock(&(queue->mutex));
         return 0;
     }
     else {
         pthread_mutex_unlock(&(queue->mutex));
         return 1;
     }
}
//for testing only
// int main() {
//   Tasks_queue tasks_list;
//   init_queue(&tasks_list);
//
//   Task_info newinfo;
//   newinfo.slave_id=1;
//   printf("%d", push(&tasks_list, &newinfo));
//   newinfo.slave_id=2;
//   printf("%d", push(&tasks_list, &newinfo));
//   newinfo.slave_id=3;
//   printf("%d\n", push(&tasks_list, &newinfo));
//   printf("%d", pop(&tasks_list, &newinfo));
//   printf("%d", newinfo.slave_id);
//   printf("%d", pop(&tasks_list, &newinfo));
//   printf("%d", newinfo.slave_id);
//   printf("%d", pop(&tasks_list, &newinfo));
//   printf("%d", newinfo.slave_id);
//
// }
