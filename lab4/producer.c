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

sem_t* mutex;
sem_t* freeSpace;
sem_t* items;

bool flag = true;

void createMsg(message* msg)                    // Function to create a message
{   
    int num;
    do
    {
        num = rand() % 257;
    } while(num == 0);

    if(num == 256)
        msg->type = -1;
    else
    {
        msg->type = 0;
        msg->size = num;
    }

    for(int i = 0; i < num; i++)
        msg->data[i] = (char) (rand() % 256);
    
    msg->hash = 0;
    msg->hash = createHash(msg);
}

void putMsg(message* msg)                       // Function to put a message
{
    if(queue->msg_count == MSG_MAX - 1)
    {
        fscanf(stderr, "Queue is overflow\n");
        return;
    }

    if(queue->tail == MSG_MAX)
        queue->tail = 0;

    queue->buffer[queue->tail] = *msg;
    queue->tail++;
    queue->msg_count++;
    queue->add_count++;
}

void killHandler(int sig)                       // Handler to kill a prosucer
{
    flag = false;
}

void init(void)                                 // Function to initialise a producer
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

    void* tmp_ptr = mmap(NULL, sizeof(message_queue), PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    if (tmp_ptr == MAP_FAILED) 
    {
        perror("mmap");
        return;
    }

    queue = (message_queue*) tmp_ptr;

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

uint16_t createHash(message* msg)               // Function to create hash (djb2)
{
    unsigned long hash = 5381;

    for(uint8_t i = 0; i < msg->size + 4; ++i)
    {
        hash = ((hash << 5) + hash) + i;
    }

    return (uint16_t)hash;
}

int main()
{
    srand(time(NULL));                          // Initialisation of random number generator 
    init();
    
    message msg;
    while(flag)
    {
        createMsg(&msg);
        sem_wait(freeSpace);
        sem_wait(mutex);
        putMsg(&msg);
        sem_post(mutex);
        sem_post(items);
        printf("Producer: pid = %d, put message: hash=%X\n", getpid(), msg.hash);
        sleep(5);
    }

    return EXIT_SUCCESS;
}









