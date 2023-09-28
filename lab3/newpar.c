#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <err.h>
#include <stdlib.h>
#include <bits/sigaction.h>
#define SA_RESTART	0x10000000
//#define __USE_POSIX199309

struct childProcess
{
    char  name[10];
    pid_t pid;
    int allow_print;
};

struct childProcess child_proc[100];
int proc_amount = 0;

char input_wait(int child_num)
{
    int value;
    struct timeval tmo;
    fd_set readf; 

    printf("\nIf you want to continue child_%d process and stop other processes input 'g'\n", child_num);
    fflush(stdout);

    FD_ZERO(&readf);
    FD_SET(0, &readf);
    tmo.tv_sec = 5;
    tmo.tv_usec = 0;

    value = select(1, &readf, NULL, NULL, &tmo);

    if (value == 0)
    {
        printf("Timer is out...\n");
        return '!';
    }
    else if (value == -1)
    {
        err(1, "select");
        return '!';
    }

    char ch;
    scanf(" %c", &ch);
    return ch;
}


int num = 0;

void add_childproc(const char* child_path)
{
    printf("Add child proc... - proc_amount - %d\n", proc_amount);
    sprintf(child_proc[proc_amount].name, "child_%d", proc_amount);            
    char* child_argv[] = {
        child_proc[proc_amount].name, NULL
    };

    pid_t pid = fork();
    if (pid == 0)
    {
        printf("Execute %s\n.", child_proc[proc_amount].name);
        
        execve(child_path, child_argv, NULL);
    }
    else if (pid > 0)
    {
        child_proc[proc_amount].pid = pid;
        printf("Call %s, with id -  %d\n", child_proc[proc_amount].name, child_proc[proc_amount].pid);
        proc_amount++;
    }

    sleep(1);
}

void kill_childproc()
{
    kill(child_proc[proc_amount - 1].pid, SIGKILL);
    
    proc_amount--;

    printf("child_%d is killed\n", proc_amount);
}

void sig_for_all(int p_signal)
{
    for (int i = 0; i < proc_amount; i++)
    {
        kill(child_proc[i].pid, p_signal);
    }
}

void show_process()
{   
    printf("Proc type\t PID\n\n");
    printf("Parent\t\t%d\n\n", getpid());

    for (int i = 0; i < proc_amount; i++)
    {
        printf("%7s  \t%d\n", child_proc[i].name, child_proc[i].pid);
    }
}

int get_num(char* c)
{
    char* c_num = c + 1;
    return atoi(c_num);
}

void atexit_handler()
{
    sig_for_all(SIGKILL);
    printf("child processes are killed.\n");
}

void init_atexit()
{
    atexit(atexit_handler);
}

void child_sigrecieve(int signal, siginfo_t* info, void* func)
{   
    if (signal == SIGUSR1)
    {
        sig_for_all(SIGSTOP);
        printf("Allow output for %d?(y/n)", info->si_pid);
        char* choice = (char*)malloc(2 * sizeof(char));
        scanf("%s", choice);
        if (!strcmp(choice, "y"))
        {
            kill(info->si_pid, SIGUSR2);
        }
        
        sig_for_all(SIGCONT);
        rewind(stdin);      
    }
}

void init_sigaction()
{
    struct sigaction child_sig;
    memset(&child_sig, 0, sizeof(child_sig));
    child_sig.sa_sigaction = &child_sigrecieve;
    child_sig.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGUSR1, &child_sig, 0);
}

void print_menu()
{
    printf("q - exit\n+ - add child\nl - list of processes\nk - kill all\ns<num> - stop all/<child>\ng<num> - resum all/<child>\n");
}

int main(int argc, char** argv)
{
    char child_path[] = "/home/viktor/workshops/work_1/child";
    char c[10];
    init_atexit();
    init_sigaction();
    print_menu();
    while(1)
    {
        rewind(stdin);
        scanf("%s", c);
        printf("\n");
        if (c[0] == 'q')
        {
            for(int i = 0; i < proc_amount; i++)
            {
                kill(child_proc[i].pid, SIGKILL);
            }

            break;
        }
        
        if (c[0] == '+')
        {
            add_childproc(child_path);
            continue;
        }

        if (c[0] == '-')
        {
            kill_childproc();
            continue;
        }

        if (c[0] == 'l')
        {
            show_process();
            continue;
        }

        if (c[0] == 'k')
        {   
            sig_for_all(SIGTERM);
            printf("\nAll child processes are killed\n");

            proc_amount = 0;
            continue;
        }

        
        if (c[0] == 's' && c[1] != 0)
        {         
            int i_num = get_num(c);

            kill(child_proc[i_num].pid, SIGUSR1);
            continue;
        }

        if (c[0] == 'g' && c[1] != 0)
        {
            int i_num = get_num(c);

            kill(child_proc[i_num].pid, SIGUSR2);
            continue;
        }

        if (c[0] == 's')
        {
            sig_for_all(SIGUSR1); //forbid

            printf("\nAll child processes are stopped\n");
            continue;
        }

        if (c[0] == 'g')
        {
            sig_for_all(SIGUSR2); //resume

            printf("\nAll child processes are resumed\n");
            continue;
        }

       
        if (c[0] == 'p' && c[1] != 0)
        {
            int i_num = get_num(c);

            sig_for_all(SIGSTOP);

            kill(child_proc[i_num].pid, SIGUSR2);

            if (input_wait(i_num) != 'g')
            {
                rewind(stdin);
                sig_for_all(SIGUSR2);
                sig_for_all(SIGCONT);
            }
            else
            {
                //kill(child_proc[i_num].pid, SIGCONT);
                sig_for_all(SIGCONT);
            }

            continue;
        }
    }

    return 0;
}
