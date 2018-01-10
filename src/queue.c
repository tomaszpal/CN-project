#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MAX_TASKS_NUMBER 2

typedef struct task_info{
    //RData_File resource;
    int slave_id;
    int client_id;
} task_info;

typedef struct Tasks_queue{
    task_info queue[MAX_TASKS_NUMBER];
    pthread_mutex_t mutex;
    int first;
    int last;
    int counter;
} Tasks_queue;

void init_queue(Tasks_queue* queue){
    pthread_mutex_init(&(queue->mutex), NULL);
    queue->first = 0;
    queue->last = -1;
    queue->counter = 0;
}

int is_full(Tasks_queue* queue) {
    pthread_mutex_lock(&(queue->mutex));
    int res = queue->counter == MAX_TASKS_NUMBER;
    pthread_mutex_unlock(&(queue->mutex));
    return res;
}

//return 1 if success, 0 if is already full
//parameters: queue - queue structure pointer
//t - pointer to task_info structure to be pushed into queue
int push(Tasks_queue* queue, task_info* t) {
   if (!is_full(queue)) {
        pthread_mutex_lock(&(queue->mutex));
        if (queue->last == MAX_TASKS_NUMBER-1)
            queue->last = -1;
        task_info* new_task = &(queue->queue[++(queue->last)]);
        memcpy(new_task, t, sizeof(task_info));
        queue->counter++;
        pthread_mutex_unlock(&(queue->mutex));
        return 1;
   }
   else
        return 0;
}

//return 1 if success, 0 if is already empty
//parameters: queue - queue structure pointer
//t - pointer to task_info structure to save popped value from queue
int pop(Tasks_queue* queue, task_info* t) {
     pthread_mutex_lock(&(queue->mutex));
     if(queue->counter != 0){
         memcpy(t, &(queue->queue[queue->first++]), sizeof(task_info));
         if (queue->first == MAX_TASKS_NUMBER) {
            queue->first = 0;
         }
         queue->counter--;
         pthread_mutex_unlock(&(queue->mutex));
         return 1;
     }
     else {
         pthread_mutex_unlock(&(queue->mutex));
         return 0;
     }
}

//for testing only
int main() {
  Tasks_queue tasks_list;
  init_queue(&tasks_list);

  task_info newinfo;
  newinfo.slave_id=1;
  printf("%d", push(&tasks_list, &newinfo));
  newinfo.slave_id=2;
  printf("%d", push(&tasks_list, &newinfo));
  newinfo.slave_id=3;
  printf("%d\n", push(&tasks_list, &newinfo));
  printf("%d", pop(&tasks_list, &newinfo));
  printf("%d", newinfo.slave_id);
  printf("%d", pop(&tasks_list, &newinfo));
  printf("%d", newinfo.slave_id);
  printf("%d", pop(&tasks_list, &newinfo));
  printf("%d", newinfo.slave_id);

}
