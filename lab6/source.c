#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>

#define INDEX_S_SIZE 16
#define UINT64_SIZE 8

struct index_s
{
    double          time_mark;
    uint64_t        recno; 
};

struct index_hdr_s
{
    uint64_t        rec_amount;
    struct index_s  idx[];
};

double get_day()
{
    return (double)(rand() % (365 * 123)) + 15020.0;
}

double get_day_time()
{

    return (double)(rand() % RAND_MAX) ? 0 : (double)(rand() % RAND_MAX); 
}

int main(int argc, char *argv[]) {

    if (argc != 3) 
    {
        printf("Input Error: ./source <file_name> <file_size_in_mb>\n");
        return -1;
    }

    if (atoi(argv[2]) <= 0 || atoi(argv[2]) >= 14000)
    {
        printf("Enter the correct size of file!\n");
        return -1;
    }

    uint64_t size = atoi(argv[2]);
    size_t rec_amount = (size * (1024*1024)) / INDEX_S_SIZE;
    uint64_t header_size = UINT64_SIZE + INDEX_S_SIZE * rec_amount;
    struct index_hdr_s* header = (struct index_hdr_s*)malloc(header_size);

    if (header == NULL)
    {
        printf("Memory allocation failed!\n");
        return -1;
    }

    header->rec_amount = rec_amount;

    for(uint64_t i = 0; i < rec_amount; i++)
    {
        header->idx[i].recno = i;
        header->idx[i].time_mark = get_day() + get_day_time();
    }

    int fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0777);

    if (fd == -1)
    {
        printf("Error file creation!\n");
        return -1;
    }

    if (ftruncate(fd, header_size) == -1) 
    {
        printf("Error setting file size!\n");
        return -1;
    }

    write(fd, header, UINT64_SIZE + INDEX_S_SIZE * rec_amount);
    free (header);
    printf("Header size %ld\n", header_size);

    close(fd);
    
    return 0;
}