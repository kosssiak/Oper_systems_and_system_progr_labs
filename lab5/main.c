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
#include <pthread.h>

#define DATA_MAX (((256 + 3) / 4) * 4)
#define CHILD_MAX 1024


typedef struct {                        // message struct 
  int type;
  int hash;
  int size;
  char data[DATA_MAX];
} message;

typedef struct {                        // message queue struct
  int add_number;                       // amount of all added messages
  int extract_number;                   // amount of all recieved messages
  int msg_number;                       // number of messages that wait and will be recieved
  int head;
  int tail;
  message* buffer;                      // buffer of messages
} message_queue;

//functions for main
void printInfoAboutQueue();             // function to display info about queue 
void init();                            // function to initialise message queue, mutex, semaphores
void atexitHandler();                   // handler-function to terminate the priogram correctly

// functions for message
int hash(message* msg);                 // function for hash creation
void msgQueueInit();                    // function for message queue initialisation
void createMessage(message* msg);       // function for message creation
void putMsg(message* msg);              // function to put message by producer
void getMsg(message* msg);              // function for getting message by consumer

// functions for producer
void createProd();                      // function for producer creation
void removeProd();                      // function for producer removal
void* prodHandler(void* arg);           // handler-function for producer

// functions for consumer
void createCons();                      // function for consumer creation
void removeCons();                      // function for consumer removal
void* consHandler(void* arg);           // handler-function for consumer

message_queue* queue;                   // message queue

pthread_mutex_t mutex;                  // mutex

sem_t* free_space;                      // semaphore
sem_t* items;                           // semaphore

pthread_t producers[CHILD_MAX];         // producers threads
int prodNumber;                         // number of producers

pthread_t consumers[CHILD_MAX];         // consumers threads
int consNumber;                         // number of consumers

bool prodFlag = true;                   // flag for producers
bool consFlag = true;                   // flag for consumers
bool overflowFlag = false;              // flag for overflow

int bufSize = 10;                       // buffer size (max number of added messages = 10)

void init(void) 
{

  if(atexit(atexitHandler))
  {
    perror("atexit");
    exit(1);
  }
  
  msgQueueInit();
  
  int res = pthread_mutex_init(&mutex, NULL);
  if (res) 
  {
    fprintf(stderr, "Failed mutex init!\n");
    exit(1);
  }

  if ((free_space = sem_open("free_space", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), bufSize)) == SEM_FAILED ||
      (items = sem_open("items", (O_RDWR | O_CREAT | O_TRUNC), (S_IRUSR | S_IWUSR), 0)) == SEM_FAILED) 
  {
    fprintf(stderr, "sem_open");
    exit(1);
  }
}

void atexitHandler()
{
  int tmp = pthread_mutex_destroy(&mutex);
  if (tmp) 
  {
    fprintf(stderr, "Failed mutex destruction!\n");
    exit(1);
  }
  
  if (sem_unlink("free_space") || sem_unlink("items")) 
  {
    fprintf(stderr, "sem_unlink");
    abort();                            // Abort execution and generate a core-dump
  }
}

int hash(message* msg) 
{
  unsigned long hash = 5381;

  for (int i = 0; i < msg->size + 4; ++i) 
  {
    hash = ((hash << 5) + hash) + i;
  }

  return (int) hash;
}

void msgQueueInit() 
{
  queue = (message_queue*)malloc(1 * sizeof(message_queue));
  
  queue->buffer = (message*)malloc(bufSize * sizeof(message));
  queue->add_number = 0;
  queue->extract_number = 0;
  queue->head = 0;
  queue->msg_number = 0;
  queue->tail = 0;
}

void putMsg(message* msg) 
{
  if (queue->msg_number == bufSize) 
  {
    overflowFlag = true;
    return;
  }

  if (queue->tail == bufSize)
    queue->tail = 0;
  
  queue->buffer[queue->tail] = *msg;
  queue->tail++;
  queue->msg_number++;
  queue->add_number++;
}

void getMsg(message* msg) 
{
  if (queue->msg_number == 0) 
  {
    fprintf(stderr, "Queue buffer overflow!\n");
    exit(1);
  }

  if (queue->head == bufSize) 
    queue->head = 0;

  *msg = queue->buffer[queue->head];
  queue->head++;
  queue->msg_number--;
  queue->extract_number++;
}

void createProd() 
{
  if (prodNumber == CHILD_MAX - 1) 
  {
    fprintf(stderr, "Max value of producers!\n");
    return;
  }

  int res = pthread_create(&producers[prodNumber], NULL, prodHandler, NULL);
  if (res) 
  {
    fprintf(stderr, "Failed to create producer!\n");
    exit(1);
  }

  ++prodNumber;
}

void removeProd() 
{
  if (prodNumber == 0) 
  {
    fprintf(stderr, "No producers to delete!\n");
    return;
  }

  --prodNumber;
  pthread_cancel(producers[prodNumber]);
  pthread_join(producers[prodNumber], NULL);
}

void createMessage(message* msg) 
{
  int value;
  do
  {
    value = rand() % 257;
  }while(value == 0);
  
  if (value == 256) 
  {
    msg->type = -1;
  } else 
  {
    msg->type = 0;
    msg->size = value;
  }

  for (int i = 0; i < value; ++i) 
  {
    msg->data[i] = (char) (rand() % 256);
  }

  msg->hash = 0;
  msg->hash = hash(msg);
}

_Noreturn void* prodHandler(void* arg) 
{
  message msg;  
  while (prodFlag) 
  {
    createMessage(&msg);

    sem_wait(free_space);

    pthread_mutex_lock(&mutex);
    putMsg(&msg);
    while(overflowFlag == true){};
    pthread_mutex_unlock(&mutex);

    sem_post(items);

    printf("%ld produce message: hash=%X\n", pthread_self(), msg.hash);

    sleep(5);
  }

  pthread_exit(NULL);
}

_Noreturn void* consHandler(void* arg) 
{
  message msg;
  while (consFlag) 
  {
    sem_wait(items);

    pthread_mutex_lock(&mutex);
    getMsg(&msg);
    pthread_mutex_unlock(&mutex);

    sem_post(free_space);

    printf("%ld consume message: hash=%X\n", pthread_self(), msg.hash);

    sleep(5);
  }

  pthread_exit(NULL);
}

void createCons() 
{
  if (consNumber == CHILD_MAX - 1) 
  {
    fprintf(stderr, "Max value of consumers!\n");
    return;
  }

  int res = pthread_create(&consumers[consNumber], NULL, consHandler, NULL);
  if (res) 
  {
    fprintf(stderr, "Failed to create producer!\n");
    exit(res);
  }

  ++consNumber;

}

void removeCons() 
{
  if (consNumber == 0) 
  {
    fprintf(stderr, "No consumers to delete!\n");
    return;
  }

  consNumber--;
  pthread_cancel(consumers[consNumber]);
  pthread_join(consumers[consNumber], NULL);
}

void printInfoAboutQueue()
{
    printf("Number of added messages: %d\n"
           "Number of extracted messages: %d\n"
           "Number of messages at this momemt: %d\n"
           "Buffer size: %d\n"
           "Info about head: %d\n"
           "Info about tail: %d\n", queue->add_number, queue->extract_number, queue->msg_number, bufSize, queue->head, queue->tail);
}

void incBufSize()
{
  pthread_mutex_lock(&mutex);
  bufSize++;

  queue->buffer = (message*)realloc(queue->buffer, bufSize * sizeof(message));
  if(queue->buffer == NULL)
  {
    printf("Failed to increase buffer size!\n");
    return;
  }
  
  sem_post(free_space);

  pthread_mutex_unlock(&mutex);
}

void decBufSize()
{
  pthread_mutex_lock(&mutex);
  
  bufSize--;

  if(bufSize == 0)
  {
    printf("Buffer size is null!\n");
    return;
  }
  else
  { 
    queue->buffer = (message*)realloc(queue->buffer, bufSize * sizeof(message));
    if(queue->buffer == NULL)
    {
      printf("Failed to reduce buffer size!\n");
      return;
    }

    if(queue->msg_number != 0){
      queue->msg_number--;
      sem_wait(items);
    }
    
    if(queue->tail != 0)
      queue->tail--;

    
  }
  
  pthread_mutex_unlock(&mutex);
}

int main()
{
    init();
    printf("%d\n", getpid());
    printf("P - create producer\n"
       "p - delete producer\n"
       "C - create consumer\n"
       "c - delete consumer\n"
       "i - print info about queue\n"
       "+ - increase size\n"
       "- - decrease size\n"
       "q - exit\n");

    while(true)
    {
        switch(getchar())
        {
            case 'P':
            {
                createProd();
                break;
            }
            case 'p':
            {
                removeProd();
                break;
            }
            case 'C':
            {
                createCons();
                break;
            }
            case 'c':
            {
                removeCons();
                break;
            }
            case 'i':
            {
                printInfoAboutQueue();
                break;
            }
            case '+':
            {
                incBufSize();
                break;
            }
            case '-':
            {
                decBufSize();
                break;
            }
            case 'q':
            {
                return 0;
            }
            default: 
                break;

        }
    }
}
