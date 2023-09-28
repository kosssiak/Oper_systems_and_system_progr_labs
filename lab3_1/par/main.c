#define _POSIX_SOURCE

#include "functions.h"

int child_amount = 0;
child childs[MAX_CHILD_AMOUNT];

void AlarmHandler()
{
    for (int i = 0; i < child_amount; i++)
    {
        if (!childs[i].print_allowed)
        {
            UnmuteAllProcesses();
            return;
        }
    }
}

int main()
{
    printf("Choose option:\n"
           "[+] - create new child process\n"
           "[-] - delete last created child process\n"
           "[l] - print all child processes\n"
           "[k] - kill all child processes\n"
           "[s] - stop print statistic for processes\n"
           "[g] - resume print statistic for processes\n"
           "[p] - print C_{num} statistics, stop other statistic\n"
           "[q] - exit\n");
    int num;
    while (true)
    {
        rewind(stdin);
        char opt = getchar();
        rewind(stdin);
        switch (opt)
        {
        case '+':
            CreateChild();
            break;
        case '-':
            KillLastChild();
            break;
        case 'l':
            StopProcesses(1);
            PrintAllProcesses();
            StopProcesses(0);
            break;
        case 'k':
            KillAllChild();
            break;
        case 's':
            StopProcesses(1);
            printf("Input number of process to stop print it statisitcs or -1 to stop print all processes statistic:\n");
            scanf("%d", &num);
            if (num == -1)
            {
                MuteAllProcesses();
            }
            else
            {
                MuteSingleProcess(num);
            }
	    StopProcesses(0);
            break;
        case 'g':
            StopProcesses(1);
            printf("Input number of process to allow print it statisitcs or -1 to allow print all processes statistic:\n");
            scanf("%d", &num);
            if (num == -1)
            {
                UnmuteAllProcesses();
            }
            else
            {
                UnmuteSingleProcess(num);
            }
	    StopProcesses(0);
            break;
        case 'p':
            printf("Input number of process to allow print it statistic:\n");
            scanf("%d", &num);
            MuteAllButOne(num);

            struct sigaction alarm_action;
            alarm_action.sa_handler = &AlarmHandler;
            sigaction(SIGALRM, &alarm_action, NULL);
            struct itimerval timer;
            timer.it_value.tv_usec = 0;
            timer.it_value.tv_sec = 5;
            setitimer(ITIMER_REAL, &timer, NULL);

            break;
        case 'q':
            KillAllChild();
            return 0;
        default:
            continue;
        }
    }

    return 0;
}
