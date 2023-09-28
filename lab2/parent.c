#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <wait.h>

extern char **environ;

char* createChildPathWithArray(char** env);	       // Функция для создания CHILD_PATH с помощью массива переменных окружения (char* envp[] либо extern char** environ)	
char* createChildPathWithGetenv();		       // Функция для создания CHILD_PATH c помощью getenv()
char* createChildName();			       // Функция для создания CHILD_NAME
char** createChildEnv(char* fenvp) ;		       // Функция для создания окружения дочернего процесса
void createChildProcess(char* childPath, char* envPath, char* childName, char* const* env);		// Функция для создания дочернего процесса
void printEnvp(char* envp[]);                          // Функция для вывода переменных окружения
int sortCmp(const void* a, const void* b);             // Функция для использования в qsort для сортировки переменных окружения

int main(int argc, char* argv[], char* envp[])
{
    if(argc != 2)
    {
        printf("Incorrect arguments count\n");
        exit(EXIT_FAILURE);
    }

    char* const* env = createChildEnv(argv[1]);
    if(env == NULL)
        return EXIT_FAILURE;

    printEnvp(envp);
    printf("+ - create new process with getenv\n"
           "* - create new process using an array of environment settings\n"
           "& - create new process using external variable\n"
           "q - quit\n");
    
    while(true)
    {
        switch(getchar())
        {
            case '+':
            {
                if(createChildPathWithGetenv() == NULL)
                    break;
                else
                {
                    createChildProcess(createChildPathWithGetenv(), argv[1], createChildName(), env);
                }
                break;
            }
            case '*':
            {
                if(createChildPathWithArray(envp) == NULL)
                {
                    printf("CHILD_PATH not found");
                    break;
                }
                else
                {
                    createChildProcess(createChildPathWithArray(envp), argv[1], createChildName(), env);
                }
                break;
            }
            case '&':
            {
                if(createChildPathWithArray(environ) == NULL)
                {
                    printf("CHILD_PATH not found");
                    break;
                }
                else
                {
                    createChildProcess(createChildPathWithArray(environ), argv[1], createChildName(), env);
                }
                break;
            }
            case 'q':
                exit(EXIT_SUCCESS);
                break;
            case '\n':
                break;
            default:
                printf("Invalid option\n");
                break; 
        }
    }

    return 0;
}

int sortCmp(const void* a, const void* b)				// Функция для использования в qsort для сортировки переменных окружения
{
    return strcoll(*(const char**)a, *(const char**)b);
}

void printEnvp(char* envp[]) 						// Функция для вывода переменных окружения
{  
  size_t envpcount = 0;
  while (envp[envpcount]) {
    ++envpcount;
  }

  setlocale(LC_COLLATE, "C");

  qsort(envp, envpcount, sizeof(char*), sortCmp);

  printf("\033[1;35m"); 					// Включаем фиолетовый цвет

  puts("Parent environment variables:");
  for (size_t i = 0; i < envpcount; ++i) 
  {
    puts(envp[i]);
  }

  printf("\033[0m"); 						// Выключаем фиолетовый цвет
}

void createChildProcess(char* childPath, char* envPath, char* childName, char* const* env)	// Функция для создания дочернего процесса		 
{
    char* const argv[] = {childName, envPath, NULL};
    pid_t pid = fork();

    if(pid < 0)
    {
      printf("Child process not created(fork)\n");
      return;
    }
    else if(pid == 0)
    {
      if(execve(childPath, argv, env) < 0)
      {
        printf("Child process not running(execve)\n");
        return;
      }
    }
    else
    {
      int status;
      waitpid(pid, &status, 0);
      if(WIFEXITED(status))
      {
        printf("Child exited with status %d\n", WEXITSTATUS(status));
      }
    }
}

char** createChildEnv(char* fenvp) 				// Функция для создания окружения дочернего процесса
{
  FILE* file = fopen(fenvp, "r");
  if (file == NULL) 
  {
    perror("fopen");
    return NULL;
  }

  char** env = malloc(sizeof(char*));
  size_t i = 0;

  int MAX_SIZE = 256;
  char buffer[MAX_SIZE];
  while (fgets(buffer, MAX_SIZE, file) != NULL) 
  {
    buffer[strcspn(buffer, "\n")] = '\0';
    char* envVal = getenv(buffer);
    if (envVal) 
    {
      env[i] = malloc((strlen(buffer) + strlen(envVal) + 2) * sizeof(char));
      strcat(strcat(strcpy(env[i], buffer), "="), envVal);
      env = realloc(env, (++i + 1) * sizeof(char*));
    }
  }
  
  env[i] = NULL;

  return env;
}

char* createChildName()					// Функция для создания CHILD_NAME
{
  static size_t childCount = 0;
  char* childName = (char*)malloc(12);
  sprintf(childName, "mychild_%zu", childCount++);

  return childName;
}

char* createChildPathWithGetenv()			// Функция для создания CHILD_PATH c помощью getenv()
{
  char* childPath = getenv("CHILD_PATH");
  if(childPath == NULL)
  {
    printf("CHILD_PATH not found\n");
  }

  return childPath;
}

char* createChildPathWithArray(char** env)		// Функция для создания CHILD_PATH с помощью массива переменных окружения (char* envp[] либо extern char** environ)
{
  const char CHILD_PATH[] = "CHILD_PATH";

  while(*env)
  {
    if(!strncmp(*env, CHILD_PATH, strlen(CHILD_PATH)))
    {
      return *env + strlen(CHILD_PATH) + 1;
    }

    ++env;
  }
  
  return NULL;
}
