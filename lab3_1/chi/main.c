#define _POSIX_SOURCE
#define SA_RESTART 0x10000000

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>

typedef struct
{
    int num1;
    int num2;
} binary;

int stat[] = {0, 0, 0, 0};
int iterations_amount = 0;
binary data;
bool is_print_allowed = true;

void AlarmHandler()
{
    int index = data.num1 * 2 + data.num2;
    stat[index]++;
    iterations_amount++;
}

void PrintHandler(int sig)
{
    if (sig == SIGUSR1)
    {
        is_print_allowed = false;
    }
    else
    {
        is_print_allowed = true;
    }
}

int main(int argc, char **argv)
{
    struct sigaction alarm_action;
    alarm_action.sa_handler = &AlarmHandler;
    sigaction(SIGALRM, &alarm_action, NULL);

    struct sigaction allow_print;
    allow_print.sa_handler = &PrintHandler;
    allow_print.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &allow_print, NULL);
    sigaction(SIGUSR2, &allow_print, NULL);

    struct itimerval timer;
    timer.it_value.tv_usec = 10000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 10000;
    timer.it_interval.tv_sec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    while (1)
    {
        data.num1 = 0;
        data.num2 = 0;
        data.num1 = 1;
        data.num2 = 1;
        if (iterations_amount == 100)
        {
            if (is_print_allowed)
            {
                char buffer[200];
                sprintf(buffer, "%s stats: pid - %d, ppid - %d, {0, 0} - %d, {0, 1} - %d, {1, 0} - %d, {1, 1} - %d.\n", argv[0], (int)getpid(), (int)getppid(),
                       stat[0], stat[1], stat[2], stat[3]);
                for (int i = 0; i < (int)strlen(buffer); i++)
                {
                    fputc(buffer[i], stdout);
                }
            }

            stat[0] = stat[1] = stat[2] = stat[3] = 0;
            iterations_amount = 0;
        }
    }

    return 0;
}