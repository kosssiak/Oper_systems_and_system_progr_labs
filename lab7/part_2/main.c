#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>

typedef struct record_s
{
    char name[80];
    char address[80];
    char semester[80];
} record_s;

int get_record_by_number(record_s *record, int record_num)
{
    int fd = open("file", O_RDONLY);
    int read_bytes;
    int current_record = 0;
    while (read_bytes = read(fd, record, sizeof(record_s)) != 0)
    {
        if (current_record == record_num)
        {
            close(fd);
            return 1;
        }

        current_record++;
    }

    close(fd);
    return -1;
}

void rewrite_record(record_s new_record, int record_num)
{
    int fd = open("file", O_RDWR);
    struct flock lock;
    lock.l_type = F_WRLCK;                                  // Тип блокировки                        
    lock.l_whence = SEEK_SET;                               // Начальное смещение (SEEK_SET - начало файла)
    lock.l_start = record_num * sizeof(record_s);           // Начальное смещение блокировки
    lock.l_len = sizeof(record_s);                          // Размер блокировки в байтах
    while (fcntl(fd, F_SETLK, &lock) == -1)
    {
        if (errno == EAGAIN)
        {
            printf("This record is blocked. Waiting...\n");
            sleep(1);
        }
        else
        {
            printf("Error has occured!\n");
            return;
        }
    }

    sleep(10);
    lseek(fd, record_num * sizeof(record_s), SEEK_SET);
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    if (write(fd, &new_record, sizeof(record_s)) > 0)
    {
        printf("Record was changed successfully!\n");
    }

    close(fd);
}

void get_all_data()
{
    int fd = open("file", O_RDONLY);
    record_s buffer;
    int read_bytes;
    while (read_bytes = read(fd, &buffer, sizeof(record_s)) != 0)
    {
        printf("Name - %s, address - %s, sem - %s\n", buffer.name, buffer.address, buffer.semester);
    }

    close(fd);
}

void print_record_by_number()
{
    record_s buffer;
    printf("Input number of record to be printed:\n");
    int record_number_to_print;
    scanf("%d", &record_number_to_print);
    if (get_record_by_number(&buffer, record_number_to_print) == 1)
    {
        printf("Name - %s, address - %s, sem - %s\n", buffer.name, buffer.address, buffer.semester);
    }
    else
    {
        printf("There is no record with given number!\n");
    }
}

void change_record()
{
    record_s buffer;
    printf("Input number of record to be changed:\n");
    int record_number_to_change;
    scanf("%d", &record_number_to_change);
    if (get_record_by_number(&buffer, record_number_to_change) == 1)
    {
        printf("Current data of the record:\n");
        printf("Name - %s, address - %s, sem - %s\n", buffer.name, buffer.address, buffer.semester);
        printf("Input new name:\n");
        rewind(stdin);
        scanf("%s", buffer.name);
        printf("Input new address:\n");
        rewind(stdin);
        scanf("%s", &buffer.address);
        printf("Input new sem:\n");
        rewind(stdin);
        scanf("%s", &buffer.semester);

        rewrite_record(buffer, record_number_to_change);
    }
    else
    {
        printf("There is no record with given number!\n");
    }
}



int main()
{
 
    printf("Choose option:\n1 - show all records\n2 - get single record\n3 - change record\n0 - exit\n");
    while (1)
    {
        int option;
        scanf("%d", &option);
        if (option == 1)
        {
            get_all_data();
        }
        else if (option == 2)
        {
            print_record_by_number();
        }
        else if (option == 3)
        {
            change_record();
        }
        else if (option == 0)
        {
            printf("Exit!\n");
            return 0;
        }
        else
        {
            printf("Wrong option!\n");
        }
    }
}
