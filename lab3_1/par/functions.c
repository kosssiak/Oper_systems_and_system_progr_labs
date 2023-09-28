#include "functions.h"

extern long child_amount;
extern child childs[MAX_CHILD_AMOUNT];
char child_processes_names[MAX_CHILD_AMOUNT][10];

void CreateChild()
{
    char child_name[8];
    sprintf(child_name, "C_%zu", child_amount);
    strcpy(child_processes_names[child_amount], child_name);
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("Process creation error!\n");
        return;
    }
    else if (pid == 0)
    {
        if (execl("child", child_name, NULL) == -1)
        {
            printf("Execl error!\n");
            return;
        }
    }
    else
    {
        childs[child_amount].pid = pid;
        childs[child_amount].print_allowed = true;
        child_amount++;
    }
}

void KillLastChild()
{
    pid_t pid_to_kill = childs[--child_amount].pid;
    kill(pid_to_kill, SIGKILL);
    printf("Process with pid %d was killed.\n", (int)pid_to_kill);
}

void KillAllChild()
{
    while (child_amount > 0)
    {
        KillLastChild();
    }

    printf("All child processes were killed!\n");
}

void PrintAllProcesses()
{
    printf("Parent process has pid %d.\n", (int)getpid());
    for (int i = 0; i < child_amount; i++)
    {
        printf("Process %s has pid %d.\n", child_processes_names[i], (int)childs[i].pid);
    }
}

void MuteAllProcesses()
{
    for (int i = 0; i < child_amount; i++)
    {
        MuteSingleProcess(i);
    }
}

void MuteSingleProcess(int process_num)
{
    pid_t pid_to_mute = childs[process_num].pid;
    childs[process_num].print_allowed = false;
    kill(pid_to_mute, SIGUSR1);
    printf("Process with pid %d was muted.\n", (int)pid_to_mute);
}

void UnmuteSingleProcess(int process_num)
{
    pid_t pid_to_unmute = childs[process_num].pid;
    childs[process_num].print_allowed = true;
    kill(pid_to_unmute, SIGUSR2);
    printf("Process with pid %d was unmuted.\n", (int)pid_to_unmute);
}

void UnmuteAllProcesses()
{
    for (int i = 0; i < child_amount; i++)
    {
        UnmuteSingleProcess(i);
    }
}

void MuteAllButOne(int num)
{
    for (int i = 0; i < child_amount; i++)
    {
        if (i != num)
        {
            MuteSingleProcess(i);
        }
    }

    printf("All processes were muted, %d wasnt muted.\n", num);
}

void StopProcesses(int flg)
{
    if (flg)
    {
        for (int i = 0; i < child_amount; i++)
        {
            kill(childs[i].pid, SIGSTOP);
        }
    }
    else
    {
        for (int i = 0; i < child_amount; i++)
        {
            kill(childs[i].pid, SIGCONT);
        }
    }
}
