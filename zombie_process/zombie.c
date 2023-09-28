#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main ()
{
	pid_t pid;
	pid = fork ();
	if (pid > 0) 
	{
		sleep (60);
	}
	else
	{
	exit (0);
	}
	return 0;
}
