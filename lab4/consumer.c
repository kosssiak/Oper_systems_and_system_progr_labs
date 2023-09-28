#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <inttypes.h>

#include "struct.h"

message_queue* queue;
message msg;

sem_t* mutex;
sem_t* freeSpace;
sem_t* items;

bool flag = true;

void init(void)                         // Function to initialise a consumer
{   
    if(signal(SIGUSR1, killHandler) == SIG_ERR)
    {
        perror("signal");
        return;
    }
    
    int fileDescriptor = shm_open("/queue", O_RDWR, (S_IRUSR | S_IWUSR));
    if (fileDescriptor < 0) 
    {
        perror("shm_open");
        return;
    }

    void* ptr = mmap(NULL, sizeof(message_queue), PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    if (ptr == MAP_FAILED) 
    {
        perror("mmap");
        return;
    }

    queue = (message_queue*) ptr;

    if(close(fileDescriptor))
    {
        perror("close");
        return;
    }

    if ((mutex = sem_open("mutex", O_RDWR)) == SEM_FAILED ||
        (freeSpace = sem_open("freeSpace", O_RDWR)) == SEM_FAILED ||
        (items = sem_open("items", O_RDWR)) == SEM_FAILED) {
        perror("sem_open");
        return;
    }
}

void getMsg(void)                       // Function to get message from producer
{
    if(queue->msg_count == 0)
        return;
    
    if(queue->head == MSG_MAX)
        queue->head;
    
    msg = queue->buffer[queue->head];
    queue->head++;
    queue->msg_count--;
    queue->extract_count++;
}

void killHandler(int sig)               // Handler to kill a consumer
{
    flag = false;
}

int main()
{
    init();

    while(flag)
    {
        sem_wait(items);
        sem_wait(mutex);
        getMsg();
        sem_post(mutex);
        sem_post(freeSpace);
        printf("Consumer: pid=%d, get message: hash=%X\n", getpid(), msg.hash);
        sleep(5);
    }

    return EXIT_SUCCESS;
}




