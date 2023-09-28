#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef struct record_s
{
    char name[80];
    char address[80];
    char semester[80];
} record_s;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incorrect input!Correct: ./file_make [filename] [count of records]\n");
        return EXIT_FAILURE;
    }

    char* fileName = argv[1];
    int countOfRecords = atoi(argv[2]);

    struct record_s arrayRecords[countOfRecords];

    for(int i = 0; i < countOfRecords; i++)
    {
        sprintf(arrayRecords[i].name, "Name_%d", i);
        sprintf(arrayRecords[i].address, "Address%d", i);
        sprintf(arrayRecords[i].semester, "%d", i);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) 
    {
        perror("open");
        exit(1);
    }

    write(fd, &arrayRecords, sizeof(arrayRecords));
    
    return EXIT_SUCCESS;
}

