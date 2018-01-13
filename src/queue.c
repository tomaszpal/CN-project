#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue.h>

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

//return 0 if success, 1 if is already full
//parameters: queue - queue structure pointer
//t - pointer to Task_info structure to be pushed into queue
int push(Tasks_queue* queue, Task_info* t) {
   if (!is_full(queue)) {
        pthread_mutex_lock(&(queue->mutex));
        if (queue->last == MAX_TASKS_NUMBER - 1) {
            queue->last = -1;
        }
        Task_info* new_task = &(queue->queue[++(queue->last)]);
        memcpy(new_task, t, sizeof(Task_info));
        queue->counter++;
        pthread_mutex_unlock(&(queue->mutex));
        return 0;
   }
   else
        return 1;
}

//return 0 if success, 1 if is already empty
//parameters: queue - queue structure pointer
//t - pointer to Task_info structure to save popped value from queue
int pop(Tasks_queue* queue, Task_info* t) {
     pthread_mutex_lock(&(queue->mutex));
     if (queue->counter != 0) {
         memcpy(t, &(queue->queue[queue->first++]), sizeof(Task_info));
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
