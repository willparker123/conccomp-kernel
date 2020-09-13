/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "console.h"

/* The following functions are special-case versions of a) writing, and 
 * b) reading a string from the UART (the latter case returning once a 
 * carriage return character has been read, or a limit is reached).
 */

void puts(char *x, int n)
{
  for (int i = 0; i < n; i++)
  {
    PL011_putc(UART1, x[i], true);
  }
}

void gets(char *x, int n)
{
  for (int i = 0; i < n; i++)
  {
    x[i] = PL011_getc(UART1, true);

    if (x[i] == '\x0A')
    {
      x[i] = '\x00';
      break;
    }
  }
}

/* Since we lack a *real* loader (as a result of also lacking a storage
 * medium to store program images), the following function approximates 
 * one: given a program name from the set of programs statically linked
 * into the kernel image, it returns a pointer to the entry point.
 */

extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_diningPhil();

void *load(char *x)
{
  if (0 == strcmp(x, "P3"))
  {
    return &main_P3;
  }
  else if (0 == strcmp(x, "P4"))
  {
    return &main_P4;
  }
  else if (0 == strcmp(x, "P5"))
  {
    return &main_P5;
  }
  else if (0 == strcmp(x, "diningPhil"))
  {
    return &main_diningPhil;
  }

  return NULL;
}

/* The behaviour of a console process can be summarised as an infinite 
 * loop over three main steps, namely
 *
 * 1. write a command prompt then read a command,
 * 2. tokenize command, then
 * 3. execute command.
 *
 * As is, the console only recognises the following commands:
 *
 * a. execute <program name>
 *
 *    This command will use fork to create a new process; the parent
 *    (i.e., the console) will continue as normal, whereas the child
 *    uses exec to replace the process image and thereby execute a
 *    different (named) program.  For example,
 *    
 *    execute P3
 *
 *    would execute the user program named P3.
 *
 * b. terminate <process ID> 
 *
 *    This command uses kill to send a terminate or SIG_TERM signal
 *    to a specific process (identified via the PID provided); this
 *    acts to forcibly terminate the process, vs. say that process
 *    using exit to terminate itself.  For example,
 *  
 *    terminate 3
 *
 *    would terminate the process whose PID is 3.
 */

void main_console()
{
  while (1)
  {
    char cmd[MAX_CMD_CHARS];

    //  step 1: write command prompt, then read command.
    puts("console$ ", 9);
    gets(cmd, MAX_CMD_CHARS);

    //  step 2: tokenize command.
    int cmd_argc = 0;
    char *cmd_argv[MAX_CMD_ARGS];

    for (char *t = strtok(cmd, " "); t != NULL; t = strtok(NULL, " "))
    {
      cmd_argv[cmd_argc++] = t;
    }

    /*  step 3: execute command. [] = required, {} = optional

        execute/exec/e [P3/P4/P5]
          -executes a user process

        fork/f [PID]
          -forks a process

        terminate/kill/k [P3/P4/P5]
          -kills a user process

        terminate-store/kill-store/ks [P3/P4/P5]
          -kills a user process and stores in history table

        priority/prio/p [P3/P4/P5]
          -increases priority to "1" - first position in top-level queue

        exit
          -kills the kernel and frees up memory

        status/tasks/s
          -shows all entries in the process table

        history/h
          -shows all killed process table entries (entries in the history table)
        
        pause/stop/p [PID]
          -pauses a process in the process table (sets priority = 0, removes from scheduler)

        unpause/continue/resume/c/start [PID]
          -unpauses a process in the process table (sets priority = 1, adds to scheduler)
    */

    //  EXECUTE/EXEC/E
    if (0 == strcmp(cmd_argv[0], "execute") || 0 == strcmp(cmd_argv[0], "exec") || 0 == strcmp(cmd_argv[0], "e"))
    {
      void *addr = load(cmd_argv[1]);

      if (addr != NULL)
      {
        if (0 == fork())
        {
          exec(addr);
        }
      }
      else
      {
        puts("unknown program\n", 16);
      }
    }

    //  TERMINATE/KILL/K
    else if (0 == strcmp(cmd_argv[0], "terminate") || 0 == strcmp(cmd_argv[0], "kill") || 0 == strcmp(cmd_argv[0], "k"))
    {
      kill(atoiLocal(cmd_argv[1]), EXIT_SUCCESS);
    }

    //  TERMINATE-STORE/KILL-STORE/KS
    else if (0 == strcmp(cmd_argv[0], "terminate-store") || 0 == strcmp(cmd_argv[0], "kill-store") || 0 == strcmp(cmd_argv[0], "ks"))
    {
      kill(atoiLocal(cmd_argv[1]), EXIT_FAILURE);
    }

    //  PAUSE/STOP/P
    else if (0 == strcmp(cmd_argv[0], "pause") || 0 == strcmp(cmd_argv[0], "p") || 0 == strcmp(cmd_argv[0], "stop"))
    {
      pause(atoiLocal(cmd_argv[1]));
    }

    //  CONTINUE/RESUME/START/C
    else if (0 == strcmp(cmd_argv[0], "continue") || 0 == strcmp(cmd_argv[0], "resume") || 0 == strcmp(cmd_argv[0], "unpause") || 0 == strcmp(cmd_argv[0], "start") || 0 == strcmp(cmd_argv[0], "c"))
    {
      unpause(atoiLocal(cmd_argv[1]));
    }

    //  STATUS/TASKS/S
    else if (0 == strcmp(cmd_argv[0], "status") || 0 == strcmp(cmd_argv[0], "s") || 0 == strcmp(cmd_argv[0], "tasks"))
    {
      status();
    }

    //  HISTORY/H
    else if (0 == strcmp(cmd_argv[0], "history") || 0 == strcmp(cmd_argv[0], "h"))
    {
      history();
    }

    //  PRIORITY/PRIO/P
    else if (0 == strcmp(cmd_argv[0], "priority") || 0 == strcmp(cmd_argv[0], "prio") || 0 == strcmp(cmd_argv[0], "p"))
    {
      nice(atoiLocal(cmd_argv[1]), 1);
    }

    //  EXIT
    else if (0 == strcmp(cmd_argv[0], "exit"))
    {
      close(EXIT_SUCCESS);
    }

    //  FORK/F
    else if ((0 == strcmp(cmd_argv[0], "fork") || 0 == strcmp(cmd_argv[0], "f")) && isdigit(cmd_argv[1][0]))
    {
      forkProc(atoiLocal(cmd_argv[1]));
    }

    else
    {
      puts("unknown command\n", 16);
    }
  }

  exit(EXIT_SUCCESS);
}
