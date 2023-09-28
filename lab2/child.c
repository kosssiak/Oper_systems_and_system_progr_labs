#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 100

int main(int argc, char* argv[], char* envp[]) 
{
  printf("\033[1;32m"); 		// Включаем зелёный цвет

  puts("Child process data:");
  printf("Name: %s\n", argv[0]);
  printf("Pid: %d\n", getpid());
  printf("Ppid: %d\n", getppid());

  printf("\033[0m"); 			// Выключаем зелёный цвет

  FILE* file = fopen(argv[1], "r");
  if(file == NULL)
  {
    perror("Error opening file\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Child environment variables:\n");

  char line[MAX_LINE_LENGTH];
  
  while (fgets(line, MAX_LINE_LENGTH, file) != NULL) 
    {
        if (line[strlen(line) - 1] == '\n') 
        {
          line[strlen(line) - 1] = '\0';
        }
        char* value = getenv(line);
        if (value != NULL) 
        {
          printf("%s=%s\n", line, value);
        } 
        else 
        {
          printf("%s not found in environment\n", line);
        }
    }

  fclose(file);

  return EXIT_SUCCESS;
}
