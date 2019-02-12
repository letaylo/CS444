#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"   /*include declarations for parse-related structs*/

#define MAX_CMD_LEN 128
#define HISTORY_COUNT 30

enum
BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT,JOBS,CD,HELP,KILL,HISTORY,REPEAT};
 
char *
buildPrompt()
{
  char cmd[1024];
  getcwd(cmd, 1024);
  printf(cmd);
  return  "%";
}

/*Struct for linked list for background processes */
typedef struct bgNode {
  int pid;
  char *command;
  struct bgNode * next;
} bgNode_t;

int
isBuiltInCommand(char * cmd){
  
  if ( strncmp(cmd, "exit", strlen("exit")) == 0){
    return EXIT;
  }else if ( strncmp(cmd, "jobs", strlen("jobs")) == 0) {
    return JOBS;
  }else if ( strncmp(cmd, "history", strlen("history")) == 0) {
    return HISTORY;
  }else if ( strncmp(cmd, "cd", strlen("cd")) == 0) {
    return CD;
  }else if ( strncmp(cmd, "help", strlen("help")) == 0) {
    return HELP;
  }else if ( strncmp(cmd, "kill", strlen("kill")) == 0) {
    return KILL;
  }else if ( strncmp(cmd, "!", strlen("!")) == 0) {
    return REPEAT;
  }
  return NO_SUCH_BUILTIN;
}

/* displays history */
int history(char *hist[], int current) {
  int i = current;
  int hist_num = 1;
  do {
    if(hist[i]) {
      printf("%4d %s\n", hist_num, hist[i]);
      hist_num++;
    }

    i = (i + 1) % HISTORY_COUNT;
  } while (i != current);

  return 0;
}

int 
main (int argc, char **argv)
{
  FILE *infile, *outfile;
  int childPid;
  char * cmdLine;
  parseInfo *info; /*info stores all the information returned by parser.*/
  struct commandType *com; /*com stores command name and Arg list for one command.*/
  bgNode_t *head = NULL;
  bgNode_t *curr = NULL;
  bgNode_t *temp = NULL;
  int i, current = 0;
  char cmd[MAX_CMD_LEN];
  char *hist[HISTORY_COUNT];
  char *num;
  int index;
  char *killPid;

  for(i = 0; i < HISTORY_COUNT; i++) {
    hist[i] = NULL;
  }
#ifdef UNIX
  
    fprintf(stdout, "This is the UNIX version\n");
#endif

#ifdef WINDOWS
    fprintf(stdout, "This is the WINDOWS version\n");
#endif

  while(1){
     
#ifdef UNIX
    cmdLine = readline(buildPrompt());
    if (cmdLine == NULL) {
      fprintf(stderr, "Unable to read command\n");
      continue;
    }
#endif

    /*calls the parser*/
    info = parse(cmdLine);
    if (info == NULL){
      free(cmdLine);
      continue;
    }
    /*prints the info struct*/
    print_info(info);

    /*com contains the info. of the command before the first "|"*/
    com=&info->CommArray[0];
    if ((com == NULL)  || (com->command == NULL)) {
      free_info(info);
      free(cmdLine);
      continue;
    }

    /*HISTORY*/
    if(isBuiltInCommand(com->command) == REPEAT){
      num = (com->command) + 1;
      index = atoi(num);
      if(index > 0) {
        index--;
        if(hist[index] != NULL) {
          info = parse(hist[index]);
          com = info->CommArray;
        } else {
          printf("Invalid Number Specified. History Unavailable\n");
        }
      }else if(index < 0) {
        index += current;
        if(hist[index] != NULL) {
          info = parse(hist[index]);
          com = info->CommArray;
        }else {
          printf("Invalid Number Specified. History Unavailable\n");
        }
      }else if(index == 0) {
        printf("Invalid Number Specified. History Unavailable\n");
      }
    }else {
      if(hist[current]) free(hist[current]);
      hist[current] = (char *) strdup(cmdLine);
      current = (current + 1) % HISTORY_COUNT;
    }
    /*com->command tells the command name of com*/
    if (isBuiltInCommand(com->command)) {
      if (isBuiltInCommand(com->command) == EXIT){
		  /* checks for background processes and updates list */
		if( head == NULL)
          exit(1);
        else {
          curr = head;
          temp = head;
            while (curr != NULL) {
              if (curr->pid == waitpid(curr->pid, NULL, WNOHANG)) {
                if (curr == head) {
                  head = head->next;
                  curr = head;
                  temp = head;
                }else {
                  temp->next = curr->next;
                  temp = curr;
                  curr = curr->next;
                  free(temp->command);
                  free(temp);
                  temp = curr;
                }
              }else {
                temp = curr;
                curr = temp->next;
              }
            }
		}
        if (head == NULL)
		  exit(1);
	    else
		  printf("Jobs Still Running! Please kill processes before exiting!\n");
      }else if(isBuiltInCommand(com->command) == CD) {
        chdir(com->VarList[1]);
      }else if(isBuiltInCommand(com->command) == JOBS) {
        if( head == NULL)
          printf("No Background Jobs running!\n");
        else {
          curr = head;
          temp = head;
            while (curr != NULL) {
              if (curr->pid == waitpid(curr->pid, NULL, WNOHANG)) {
                if (curr == head) {
                  head = head->next;
                  curr = head;
                  temp = head;
                }else {
                  temp->next = curr->next;
                  temp = curr;
                  curr = curr->next;
                  free(temp->command);
                  free(temp);
                  temp = curr;
                }
              }else {
                printf("%d\t%s\n", curr->pid, curr->command);
                temp = curr;
                curr = temp->next;
              }
            }
         }
      }else if(isBuiltInCommand(com->command) == HISTORY) {
        history(hist,current);
      }else if(isBuiltInCommand(com->command) == KILL) {
        killPid = com->VarList[1];
        killPid++;
        kill(atoi(killPid), SIGKILL);
      }else if(isBuiltInCommand(com->command) == HELP) {
        printf("jobs - displays command and corresponding PID\n");
		printf("cd - changes working directory\n");
		printf("history - print previously executed commands in a numbered order\n");
		printf("! - repeats numbered command in displayed in history command\n");
		printf("exit - terminates shell process\n");
      }
    }
	/* Runs non built in commands */
    else {
      childPid = fork();
      if (childPid == 0) {
        if ( info->boolInfile ) {
	  infile = fopen(info->inFile, "r");
          dup2(fileno(infile), 0);
        }
        if ( info->boolOutfile ) {
          outfile = fopen(info->outFile, "w");
          dup2(fileno(outfile), 1);
        }
        execvp(com->command, com->VarList);
        printf("exec failed\n");
        exit(1);
      }else {
		  /* populates background cmd linked list */
        if (info->boolBackground) {          
          temp = malloc(sizeof(bgNode_t));
          temp->pid = childPid;
          temp->command = strdup(com->command);
          temp->next = head;
          head = temp;
        } else { 
          waitpid(childPid, NULL, 0);
        }
      }
    } 
    free_info(info);
    free(cmdLine);
  }/* while(1) */
}
  





