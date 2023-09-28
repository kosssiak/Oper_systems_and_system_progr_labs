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

#define CHILD_MAX 1024

message_queue* queue;

pid_t producers[CHILD_MAX];
size_t producersAmount = 0;

pid_t consumers[CHILD_MAX];
size_t consumersAmount = 0;

sem_t* mutex;
sem_t* freeSpace;
sem_t* items;

void atexitHandler(void)                    // Handler to exit a program correctly with killing all producers and consumers 
                                            // and unlinking shm and sem
{
    for(size_t i = 0; i < producersAmount; i++)
    {
        printf("Producer: pid=%d was killed\n", producers[i]);
        kill(producers[i], SIGKILL);
        wait(NULL);
    }

    for(size_t i = 0; i < consumersAmount; i++)
    {
        printf("Consumer: pid=%d was killed\n", consumers[i]);
        kill(consumers[i], SIGKILL);
        wait(NULL);
    }

    if(shm_unlink("/queue"))
    {
        perror("shm_unlink");
        abort();
    }

    if (sem_unlink("mutex") || sem_unlink("freeSpace") || sem_unlink("items")) 
    {
        perror("sem_unlink");
        abort();
    }
}

void messageQueueInit(void)                 // Function to initialise fields of queue
{
    queue->add_count     = 0;
    queue->extract_count = 0;
    queue->msg_count = 0;
    queue->head = 0;
    queue->tail = 0;
    memset(queue->buffer, 0, sizeof(queue->buffer));
}

void init(void)                             // Function to initialise
{
    if(atexit(atexitHandler))
    {
        perror("atexit");
        return;
    }
    
    int fileDescriptor = shm_open("/queue", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR)); 
    if (fileDescriptor < 0) 
    {
        perror("shm_open");
        return;
    }

    if (ftruncate(fileDescriptor, sizeof(message_queue))) 
    {
        perror("ftruncate");
        return;
    }

    void* ptr = mmap(NULL, sizeof(message_queue), PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0); // map_shared - other processes can see it 
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

    messageQueueInit();

    if ((mutex = sem_open("mutex", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 1)) == SEM_FAILED ||
      (freeSpace = sem_open("freeSpace", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), MSG_MAX)) == SEM_FAILED ||
      (items = sem_open("items", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 0)) == SEM_FAILED) 
    {
        fprintf(stderr, "sem_open");
        exit(1);
    }
}



void createProducer(void)                   // Function to create a producer
{
    if(producersAmount == CHILD_MAX - 1)
    {
        fprintf(stderr, "Producers is overflow");
        return;
    }

    pid_t pid = fork();

    if(pid == 0)
    {
        char child_name[8];
        sprintf(child_name, "P_%zu", producersAmount);
      
        if (execl("./producer", child_name, NULL) == -1) {
            perror("execl");
            return;
        }
    }
    else if(pid < 0)
    {
        perror("fork");
        return;
    }
    else
    {
        producers[producersAmount] = pid;
        producersAmount++;
    }
}

void createConsumer(void)                   // Function to create a consumer
{
    if(consumersAmount == CHILD_MAX - 1)
    {
        fprintf(stderr, "Consumers is overflow\n");
        return;
    }

    pid_t pid = fork();

    if(pid == 0)
    {
        char child_name[8];
        sprintf(child_name, "C_%zu", producersAmount);
      
        if (execl("./consumer", child_name, NULL) == -1) {
            perror("execl");
            return;
        }
    }
    else if(pid < 0)
    {
        perror("fork");
        return;
    }
    else
    {
        consumers[consumersAmount] = pid;
        consumersAmount++;
    }
}

void printInfoAboutQueue(void)              // Function to print info about queue  
{
    printf("Count of added messages: %d\n"
           "Count of extracted messages: %d\n"
           "Count of messages at this moment: %d\n"
           "Info about head: %d\n"
           "Info about tail: %d\n", queue->add_count, queue->extract_count, queue->msg_count, queue->head, queue->tail);
}

void deleteProducer(void)                   // Function to delete a producer 
{
    if(producersAmount == 0)
    {
        fprintf(stderr, "There are no producers\n");
        return;
    }

    --producersAmount;
    printf("Producer: pid %d was killed\n", producers[producersAmount]);
    kill(producers[producersAmount], SIGUSR1);
    wait(NULL);
}

void deleteConsumer(void)                   // Function to delete a consumer
{
    if(consumersAmount == 0)
    {
        fprintf(stderr, "There are no consumers\n");
        return;
    }

    --consumersAmount;
    printf("Consumer: pid %d was killed\n", consumers[consumersAmount]);
    kill(consumers[consumersAmount], SIGUSR1);
    wait(NULL);
}

int main()
{
    init();
    printf("P - create producer\n"
           "p - delete producer\n"
           "C - create consumer\n"
           "c - delete consumer\n"
           "i - print info about queue\n"
           "q - exit\n");

    while(true)
    {
        switch(getchar())
        {
            case 'P':
            {
                createProducer();
                break;
            }
            case 'p':
            {
                deleteProducer();
                break;
            }
            case 'C':
            {
                createConsumer();
                break;
            }
            case 'c':
            {
                deleteConsumer();
                break;
            }
            case 'i':
            {
                printInfoAboutQueue();
                break;
            }
            case 'q':
            {
                return EXIT_SUCCESS;
            }
            default:
                break;
        }
    }
}













