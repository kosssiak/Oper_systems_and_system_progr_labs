#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <bits/sigaction.h>
#define SA_RESTART	0x10000000

int allow_output = 0;

int stat[4] = {0, 0, 0, 0};

struct mini_map map;

int count = 0;

struct mini_map
{
    int first_it;
    int second_it;
};

void init_timer(struct itimerval* timer)
{
    timer->it_value.tv_sec = 0;
    timer->it_value.tv_usec = 10000;
    timer->it_interval.tv_sec = 0;
    timer->it_interval.tv_usec = 10000;

    setitimer(ITIMER_REAL, timer, NULL);
}

void set_printallow(int signal)
{
    if (signal == SIGUSR1) 
    {
        allow_output = 0;
    }
    else
    {
        allow_output = 1;
    }
}

void init_sigaction()
{
    struct sigaction allow_print, forbid_print;

    allow_print.sa_flags = SA_RESTART;
    allow_print.sa_handler = set_printallow;

    forbid_print.sa_flags = SA_RESTART;
    forbid_print.sa_handler = set_printallow;

    sigaction(SIGUSR2, &allow_print, NULL);
    sigaction(SIGUSR1, &forbid_print, NULL);
}

void timer_handler()
{
    int map_num = (map.first_it << 1) | map.second_it;
    stat[map_num]++; 
    count++;
}

void set_timer_handler()
{
    struct sigaction timer_sig;
    memset(&timer_sig, 0, sizeof(timer_sig));
    timer_sig.sa_handler = &timer_handler;
    sigaction(SIGALRM, &timer_sig, NULL);
}

void print_stat(char* child_name)
{
    char buf[200];
    char* stats = buf;
    sprintf(buf, "parent pid = %d\t%s pid = %d\tstatistic - [ 00 - '%2d', 01 - '%2d', 10 - '%2d', 11 - '%2d' ]\n",
            getppid(), child_name, getpid(), stat[0], stat[1], stat[2], stat[3]);
    
    while(*stats)
    {
        fputc(*stats++, stdout);
    }
}

void atexit_handler()
{
    printf("child %d is processed.", getpid());
}

void init_atexit()
{
    atexit(atexit_handler);
}

void ask_to_print()
{
    kill(getppid(), SIGUSR1);
}

int main(int argc, char** argv)
{
    struct itimerval timer;
    printf("Start child\n");
    init_atexit(); 
    init_sigaction();
    set_timer_handler();
    init_timer(&timer);
    ask_to_print();
    while(1)
    {
        map.first_it  = 0;

        map.second_it = 0;

        map.first_it  = 1;

        map.second_it = 1;


        if (count == 100)
        {        
            if (allow_output)
            {
                print_stat(argv[0]);
            }
                
            count = 0;
            for (int i = 0; i < 4; i++)
            {
                stat[i] = 0;
            }
            
        }        
    }

    return 0;
}
