#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_THREADS 63159

struct index_s 
{
    double time_mark;
    uint64_t recno;
};

struct index_hdr_s 
{
    uint64_t records;
    struct index_s idx[];
};

struct sort_block 
{
    int num;
    void *start;
    size_t size;
    bool free;
};

int memsize;
int granul;
int threads;
char *fileName;

void *fileData;
size_t fileSize;

pthread_mutex_t mutex;
pthread_t tid[MAX_THREADS];

struct sort_block *blocks;

size_t blockSize;

size_t records;
struct index_s *data;

bool all = false;
bool fl = true;
bool fl2 = true;

int cmpIndex(const void *a, const void *b)
{
    struct index_s *ia = (struct index_s *)a;
    struct index_s *ib = (struct index_s *)b;
    if (ia->time_mark < ib->time_mark) {
        return -1;
    } else if (ia->time_mark > ib->time_mark) {
        return 1;
    } else {
        return 0;
    }
}


void *sortThread(void *arg)
{
    int num = *(int*)arg;
    printf("Thread number: %d\n", num);

    struct sort_block *block = &blocks[num];
    block->free = false;
    printf("Data before in thread %d: %p %zu %d %d\n",num, block->start, block->size, block->num, block->free);

    struct index_s *dt = (struct index_s*) block->start;
    printf("%zu %lf \n", dt->recno, dt->time_mark);

    size_t countOfStructs = block->size / sizeof(struct index_s);
    qsort(dt, countOfStructs, sizeof(struct index_s),
          (int (*)(const void *, const void *)) cmpIndex);

    printf("Data after in thread %d: %p %zu %d %d\n",num, block->start, block->size, block->num, block->free);
    printf("%zu %lf \n", dt->recno, dt->time_mark);

    while(true)
    {
        pthread_mutex_lock(&mutex);

        all = true;
        for(int i = 0; i < granul; i++)
        {
            if(blocks[i].free == true)
            {
                all = false;
                block = &blocks[i];
                printf("Thread %d find block %d\n", num, block->num);
                block->free = false;
                break;
            }
        }

        pthread_mutex_unlock(&mutex);

        if(all)
            pthread_exit(NULL);

        dt = (struct index_s*) block->start;
        qsort(dt, countOfStructs, sizeof(struct index_s), (int (*)(const void *, const void *)) cmpIndex);

        printf("Data after in thread %d: %p %zu %d %d\n",num, block->start, block->size, block->num, block->free);
        printf("%zu %lf \n", dt->recno, dt->time_mark);
    }
}

void* mergeBlocks(void* args)
{
    int num1, num2;
    bool num_fl = true;
    bool isAll = false;

    while(true)
    {
        pthread_mutex_lock(&mutex);
        printf("thread %d is founding his pair\n", *(int*)args);

        for(int i = 0; i < granul; i++)
        {
            isAll=false;
            if(blocks[i].free == false)
            {
                if(num_fl)
                {
                    num1 = blocks[i].num;
                    printf("He found num1 = %d\n", num1);
                    blocks[i].free = true;
                    num_fl = false;
                }
                else
                {
                    num2 = blocks[i].num;
                    printf("He found num2 = %d\n", num2);
                    blocks[i].free = true;
                    num_fl = true;
                    isAll = true;
                    break;
                }
            }
        }

        pthread_mutex_unlock(&mutex);

        if(!isAll)
        {
            printf("thread %d found nothing\n", *(int*)args);
            pthread_exit(NULL);
        }

        size_t offset1 = blocks[num1].size;
        size_t offset2 = blocks[num2].size;

        memcpy(blocks[num1].start+offset1, blocks[num2].start, offset2);

        printf("Thread %d merge %d to %d \n", *(int*)args, num2, num1);
        blocks[num1].size= offset1 + offset2;
        blocks[num1].free = false;

        if(offset1+offset2 == memsize)
        {
            printf("Thread %d see, that 0 block is over", *(int*)args);
            pthread_exit(NULL);
        }
    }
}

void sortIndex()
{
    for (int i = 0; i < granul; i++)
    {
        blocks[i].start = data+(i * blockSize)/sizeof(struct index_s);
        struct index_s* dt = (struct index_s*)(blocks[i].start);
        printf("Data details in block i = %d: %zu %lf\n", i, dt->recno, dt->time_mark);
        blocks[i].size = blockSize;
        blocks[i].free = true;
        blocks[i].num = i;
    }

    for (int i = 0; i < threads; i++)
    {
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&tid[i], NULL, sortThread, arg);
    }

    for (int i = 0; i < threads; i++)
    {
        pthread_join(tid[i], NULL);
    }

    for (int i = 0; i < threads; i++)
    {
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&tid[i], NULL, mergeBlocks, arg);
    }
    
    for (int i = 0; i < threads; i++) {
        pthread_join(tid[i], NULL);
    }
}

void init()
{
    int res = pthread_mutex_init(&mutex, NULL);
    if (res)
    {
        fprintf(stderr, "Failed mutex init \n");
        exit(1);
    }

    blockSize = memsize / granul;
    printf("Block size: %zu", blockSize);

    blocks = (struct sort_block*)malloc(sizeof(struct sort_block)*granul);

    struct index_hdr_s *header = (struct index_hdr_s *) fileData;
    printf("\nCount of records: %zu\n", header->records);
    records = header->records;
    data = header->idx;
}

bool isPowerOfTwo(unsigned int number)
{
    return (number != 0) && ((number & (number - 1)) == 0);
}

bool checkMemorySize()
{
    int fileSizeInMb = fileSize / 1048576;
    if(memsize > fileSizeInMb)
    {
        printf("Memory size is bigger then file size\n");
        return false;
    }
    else if(memsize == fileSizeInMb)
    {
        memsize = fileSize;
        return true;
    }
    else
    {
        memsize = (memsize * 1048576) + 8;
        return true;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Incorrect input!Correct: ./sortIndex [memory size] [count of blocks] [threads] [fileName]\n");
        return EXIT_FAILURE;
    }

    memsize = atoi(argv[1]);
    granul = atoi(argv[2]);
    threads = atoi(argv[3]);
    fileName = argv[4];

    if(!isPowerOfTwo(granul))
    {
        printf("Count of blocks is not power of two\n");
        return EXIT_FAILURE;
    }

    if(threads > granul)
    {
        printf("Count of threads bigger then count of blocks\n");
        return EXIT_FAILURE;
    }

    int fd = open(fileName, O_RDWR);
    if (fd < 0)
    {
        perror("Error opening file");
        exit(1);
    }

    fileSize = lseek(fd, 0, SEEK_END);
    if(!checkMemorySize())
        return EXIT_FAILURE;

    printf("Memory size: %d\nFile size: %zu\n", memsize, fileSize);

    fileData = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fileData == MAP_FAILED)
    {
        perror("Error mapping file");
        close(fd);
        exit(1);
    }

    init();

    sortIndex();

    if (munmap(fileData, memsize) < 0)
        perror("Error unmapping file");

    close(fd);

    printf("Success\n");

    return EXIT_SUCCESS;
}