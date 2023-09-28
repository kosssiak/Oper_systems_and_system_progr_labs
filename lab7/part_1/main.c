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
#define message_MAX 4096
#define CHILD_MAX 1024

//message
typedef struct {
  int type;
  int hash;
  int size;
  char data[DATA_MAX];
} message;

typedef struct {
  int add_count;
  int extract_count;

  int message_count;

  int head;
  int tail;
  message* buffer;
} message_queue;


//message
int hash(message* _message);
void messageQueueInit();
void putMessage(message* _message);
void getMessage(message* _message);

//main
void printInfoAboutQueue();
void init();
void atexitHandler();

//producer
void createProducer();
void removeProducer();
void createMessage(message* _message);
void* producersHandler(void* arg);

//consumer
void createConsumer();
void removeConsumer();
void* consumersHandler(void* arg);

message_queue* queue;
pthread_mutex_t mutex;

pthread_cond_t producerCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t consumerCond = PTHREAD_COND_INITIALIZER;

pthread_t  producers[CHILD_MAX];
int producersAmount;

pthread_t  consumers[CHILD_MAX];
int consumersAmount;

int bufferSize = 1024;

void init(void) 
{

  if(atexit(atexitHandler))
  {
    perror("atexit");
    exit(1);
  }
  
  messageQueueInit();
  
  int res = pthread_mutex_init(&mutex, NULL);
  if (res) 
  {
    fprintf(stderr, "Failed mutex init \n");
    exit(EXIT_FAILURE);
  }

  res = pthread_cond_init(&producerCond, NULL);
  if (res != 0) 
  {
    fprintf(stderr, "Error initializing cond_producer\n");
    exit(EXIT_FAILURE);
  }
    
  res = pthread_cond_init(&consumerCond, NULL);
  if (res != 0) 
  {
    fprintf(stderr, "Error initializing cond_consumer\n");
    exit(EXIT_FAILURE);
  }
}

void atexitHandler()
{
  while(producersAmount)
  {
    --producersAmount;
    if (pthread_cancel(producers[producersAmount]) != 0) 
    {
        fprintf(stderr, "Failed to cancel producer\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(producers[producersAmount], NULL) != 0) 
    {
        fprintf(stderr, "Failed to join producer\n");
        exit(EXIT_FAILURE);
    }
  }

  while(consumersAmount)
  {
    --consumersAmount;
    if (pthread_cancel(consumers[consumersAmount]) != 0) 
    {
        fprintf(stderr, "Failed to cancel consumer\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_join(consumers[consumersAmount], NULL) != 0) 
    {
        fprintf(stderr, "Failed to join consumer\n");
        exit(EXIT_FAILURE);
    }
  }
  
  int res = pthread_mutex_destroy(&mutex);
  if (res) 
  {
    fprintf(stderr, "Failed mutex destroy\n");
    exit(1);
  }
  
  res = pthread_cond_destroy(&producerCond);
  if (res != 0) 
  {
    fprintf(stderr, "Failed to destroy cond_producer: %s\n", strerror(res));
    exit(EXIT_FAILURE);
  }

  res = pthread_cond_destroy(&consumerCond);
  if (res != 0) 
  {
    fprintf(stderr, "Failed to destroy cond_consumer: %s\n", strerror(res));
    exit(EXIT_FAILURE);
  }
}

int hash(message* _message) 
{
  unsigned long hash = 5381;

  for (int i = 0; i < _message->size + 4; ++i) 
  {
    hash = ((hash << 5) + hash) + i;
  }

  return (int) hash;
}

void messageQueueInit() 
{
  queue = (message_queue*)malloc(1 * sizeof(message_queue));
  
  queue->buffer = (message*)malloc(bufferSize * sizeof(message));
  queue->add_count = 0;
  queue->extract_count = 0;
  queue->head = 0;
  queue->message_count = 0;
  queue->tail = 0;
}

void putMessage(message* _message) 
{
  if (queue->tail == bufferSize)
    queue->tail = 0;
  
  queue->buffer[queue->tail] = *_message;
  queue->tail++;
  queue->message_count++;
  queue->add_count++;
}

void getMessage(message* _message) 
{
  if (queue->message_count == 0) 
  {
    fprintf(stderr, "Queue buffer overflow\n");
    exit(1);
  }

  if (queue->head == bufferSize) 
    queue->head = 0;

  *_message = queue->buffer[queue->head];
  queue->head++;
  queue->message_count--;
  queue->extract_count++;
}

void createProducer() 
{
  if (producersAmount == CHILD_MAX - 1) 
  {
    fprintf(stderr, "Max value of producers\n");
    return;
  }

  int res = pthread_create(&producers[producersAmount], NULL, producersHandler, NULL);
  if (res) 
  {
    fprintf(stderr, "Failed to create producer\n");
    exit(1);
  }

  ++producersAmount;
}

void removeProducer() 
{
  if (producersAmount == 0) 
  {
    fprintf(stderr, "No producers to delete\n");
    return;
  }

  --producersAmount;
  pthread_cancel(producers[producersAmount]);
  pthread_join(producers[producersAmount], NULL);
}

void createMessage(message* _message) 
{
  int value;
  do
  {
    value = rand() % 257;
  }while(value == 0);
  
  if (value == 256) 
  {
    _message->type = -1;
  } else 
  {
    _message->type = 0;
    _message->size = value;
  }

  for (int i = 0; i < value; ++i) 
  {
    _message->data[i] = (char) (rand() % 256);
  }

  _message->hash = 0;
  _message->hash = hash(_message);
}

_Noreturn void* producersHandler(void* arg) 
{
  message _message;  
  while(true) 
  {
    createMessage(&_message);

    pthread_mutex_lock(&mutex);

    while (queue->message_count == bufferSize) {
            pthread_cond_wait(&producerCond, &mutex);       // Ждём сигнал на условной переменной
    }

    putMessage(&_message);
    pthread_cond_signal(&consumerCond);                     // Отправляем сигнал потоку
    pthread_mutex_unlock(&mutex);

    printf("%ld produce message: hash=%X\n", pthread_self(), _message.hash);

    sleep(5);
  }

  pthread_exit(NULL);
}

void createConsumer() 
{
  if (consumersAmount == CHILD_MAX - 1) 
  {
    fprintf(stderr, "Max value of consumers\n");
    return;
  }

  int res = pthread_create(&consumers[consumersAmount], NULL, consumersHandler, NULL);
  if (res) 
  {
    fprintf(stderr, "Failed to create producer\n");
    exit(res);
  }

  ++consumersAmount;

}

void removeConsumer() 
{
  if (consumersAmount == 0) 
  {
    fprintf(stderr, "No consumers to delete\n");
    return;
  }

  consumersAmount--;
  pthread_cancel(consumers[consumersAmount]);
  pthread_join(consumers[consumersAmount], NULL);
}

_Noreturn void* consumersHandler(void* arg) 
{
  message  _message;
  while (true) 
  {

    pthread_mutex_lock(&mutex);

    while (queue->message_count == 0) {
            pthread_cond_wait(&consumerCond, &mutex);       // Ждём сигнал на условной переменной
    }

    getMessage(&_message);
    pthread_cond_signal(&producerCond);                     // Отправляем сигнал потоку
    pthread_mutex_unlock(&mutex);

    printf("%ld consume message: hash=%X\n", pthread_self(), _message.hash);

    sleep(5);
  }

  pthread_exit(NULL);
}

void printInfoAboutQueue()
{
    printf("Count of added messages: %d\n"
           "Count of extracted messages: %d\n"
           "Count of messages in this momemt: %d\n"
           "buffer size: %d\n"
           "Info about head: %d\n"
           "Info about tail: %d\n", queue->add_count, queue->extract_count, queue->message_count, bufferSize, queue->head, queue->tail);
}

int main()
{
    init();
    
    printf("P - to create producer\n"
       "p - to delete producer\n"
       "C - to create consumer\n"
       "c - to delete consumer\n"
       "u - print info about Queue\n"
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
                removeProducer();
                break;
            }
            case 'C':
            {
                createConsumer();
                break;
            }
            case 'c':
            {
                removeConsumer();
                break;
            }
            case 'u':
            {
                printInfoAboutQueue();
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