#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define INDEX_RECORD_SIZE 16
#define HEADER_SIZE 8

struct index_s {
    double time_mark;
    uint64_t recno;
};

struct index_hdr_s {
    uint64_t records;
    struct index_s idx[];
};

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incorrect input! Correct: ./program_name [fileName] [size in MB]\n");
        return EXIT_FAILURE;
    }

    char *fileName = argv[1];
    int size = atoi(argv[2]);
    int numberOfRecords = (size * 1024 * 1024) / INDEX_RECORD_SIZE;
    int headerSize = HEADER_SIZE + numberOfRecords * INDEX_RECORD_SIZE;

    struct index_hdr_s *header = (struct index_hdr_s *)malloc(headerSize);
    if (header == NULL)
    {
        printf("Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    header->records = numberOfRecords;

    srand(time(NULL));
    for (int i = 0; i < numberOfRecords; i++)
    {
        struct index_s *record = &(header->idx[i]);

        double days_since_1900 = (double) (rand() % (365 * 123)) + 15020.0;
        double fraction_of_day = (double) rand() / RAND_MAX / 2.0;
        record->time_mark = days_since_1900 + fraction_of_day;

        record->recno = i;
    }

    FILE *fp = fopen(fileName, "wb");
    if (fp == NULL) {
        printf("Failed to open file %s\n", fileName);
        return 1;
    }

    fwrite(header, headerSize, 1, fp);
    fclose(fp);

    printf("Generated %d index records in file %s\n", numberOfRecords, fileName);

    return 0;
}