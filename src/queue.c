#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_REQUESTS_NUMBER 6

typedef struct request_info{
    RData_File resource;
    int slave_id;
    int client_id;
} request_info;

typedef struct Request_queue{
    request_info queue[MAX_REQUESTS_NUMBER];
    pthread_mutex_t mutex;
    int first;
    int last;
    int counter;
} Reqest_queue;

void init_queue(Request_queue* req){
    Request_queue req;
    pthread_mutex_init(&(req.mutex), NULL);
    req.first = 0;
    req.last = -1;
    req.counter = 0;
}

int is_full(*Request_queue queue) {
    pthread_mutex_lock(&(queue->mutex));
    int res = queue->counter == MAX_REQUESTS_NUMBER;
    pthread_mutex_unlock(&(queue->mutex));
    return res;
}

//return 1 if success, 0 if is already full
int push(*Request_queue queue, *request_info req) {
   if(!isFull(queue)) {
        pthread_mutex_lock(&(queue->mutex));
        if(queue->last == MAX_REQUESTS_NUMBER-1)
            queue->last = -1;
        request_info* new_req = queue->queue[++last];
        memcpy(new_req, req, sizeof(request_info));
        queue->counter++;
        pthread_mutex_unlock(&(queue->mutex));
        return 1;
   }
   else
        return 0;
}

request_info pop() {
   int data = intArray[front++];
   if(front == MAX) {
      front = 0;
   }
   itemCount--;
   return data;
}

int main() {
  Request_queue requests_list;
  init_queue(&requests_list);

  //inserting new value
  request_info newinfo;
  push(&requests_list, &newinfo)

   while(!isEmpty()) {
      int n = removeData();
      printf("%d ",n);
   }
}
