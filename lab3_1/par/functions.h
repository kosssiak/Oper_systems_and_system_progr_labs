#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_CHILD_AMOUNT 1024

typedef struct
{
    pid_t pid;
    bool print_allowed;
} child;

void CreateChild();
void KillLastChild();
void KillAllChild();
void PrintAllProcesses();
void MuteSingleProcess(int process_num);
void MuteAllProcesses();
void UnmuteSingleProcess(int process_num);
void UnmuteAllProcesses();
void MuteAllButOne(int num);
void StopProcesses(int flg);

#endif