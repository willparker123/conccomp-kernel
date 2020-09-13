/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */
#include "processTable.h"
#include <stdlib.h>

pcb_t *currentProc = NULL;
pcb_t *procTable;
int procTabSize = 1;

//  number of processes in the procTable
int PROCS = 0;
//  number of active processes in the procTable (priority != 0, status != waiting)
int PROCS_ACTIVE = 0;
//  maximum number of procTable entries
int MAX_PROCS = 32;

/*  makes sure all process table entries are adjacent 
      starting at procTable[1] as procTable[0] is reserved for the console.
    
    O(n^2)  */
void procDefrag()
{
  for (int i = 0; i < procTabSize; i++)
  {
    if (procTable[i].pid == 0)
    {
      for (int j = i + 1; j < procTabSize; j++)
      {
        if (procTable[j].pid != 0)
        {
          memcpy(&procTable[i], &procTable[j], sizeof(pcb_t)); // set procTable[i] to procTable[j]
          memset(&procTable[j], 0, sizeof(pcb_t));             // set procTable[j] to 0
          break;
        }
      }
    }
  }
  return;
}

//  returns index of PID in the procTable (-1 if not in procTable)
int procTableContains(pid_t pid)
{
  for (int i = 0; i < procTabSize; i++)
  {
    if (procTable[i].pid == pid)
    {
      return i;
    }
  }
  return -1;
}

//  makes a fork (exact copy inc. context) of a process
//  MAKE CHECK FOR PID = -1 AS IT IS RETURNING PID -1 FOR CONSOLE
int procCopy(pcb_t *parentProc, ctx_t *ctx)
{
  //parentProc->status = STATUS_WAITING;
  //  if the process table is full, resize
  if (PROCS >= procTabSize && PROCS < MAX_PROCS)
  {
    procTabSize = procTabSize * 2;
    procTable = realloc(procTable, procTabSize * sizeof(pcb_t));
  }
  //  copy PCB
  memcpy(&procTable[PROCS], parentProc, sizeof(pcb_t));
  //  set new PID
  procTable[PROCS].pid = PROCS;
  //  calculate new TOS for new stack
  procTable[PROCS].tos = procTable[0].tos - (PROCS * 0x00001000);
  //  copy stack // - 0x00001000
  memcpy((uint32_t *)procTable[PROCS].tos - 0x00001000, (uint32_t *)parentProc->tos - 0x00001000, (uint32_t)0x00001000);
  memcpy(&procTable[PROCS].ctx, &parentProc->ctx, sizeof(ctx_t));
  //  clear GPR
  //  return 0 in child (forked process)
  procTable[PROCS].ctx.sp = procTable[PROCS].tos - (procTable[PROCS].tos - ctx->sp);
  procTable[PROCS].ctx.pc = ctx->pc;
  procTable[PROCS].ctx.gpr[0] = 0;
  procTable[PROCS].priority = 1;
  PROCS++;
  procTable[PROCS - 1].ctx.cpsr = 0x50;
  return procTable[PROCS - 1].pid;
}

//  initialises PCB entry in procTable and initialises procTable
//    -dynamically resizing
int procInit(void *mainFunc)
{
  //  if the process table is full, resize
  if (PROCS >= procTabSize && PROCS < MAX_PROCS)
  {
    procTabSize = procTabSize * 2;
    procTable = realloc(procTable, procTabSize * sizeof(pcb_t));
  }
  //  if this is the first entry, malloc
  else if (PROCS == 0)
  {
    procTable = malloc(procTabSize * sizeof(pcb_t));
  }

  procTable[PROCS].status = STATUS_CREATED;

  // PID is -1 for console.c, 1 for first process and incremented for every subsequent process
  if (PROCS == 0)
  {
    procTable[PROCS].pid = -1;
  }
  else if (PROCS == 1)
  {
    procTable[PROCS].pid = 1;
  }
  else
  {
    procTable[PROCS].pid = PROCS;
  }

  if (PROCS == 0)
  { //  set TOS, pc as entrypoint as SP as TOS
    procTable[PROCS].tos = (uint32_t)(&tos_user);
    procTable[PROCS].ctx.pc = (uint32_t)(mainFunc);
    procTable[PROCS].ctx.sp = procTable[0].tos;
  }

  if (PROCS != 0)
  {
    procTable[PROCS].ctx.pc = (uint32_t)(mainFunc);
    procTable[PROCS].tos = procTable[0].tos - (PROCS * 0x00001000);
    procTable[PROCS].ctx.sp = procTable[PROCS].tos;
  }

  procTable[PROCS].priority = 1;
  procTable[PROCS].status = STATUS_READY;
  PROCS++;
  procTable[PROCS - 1].ctx.cpsr = 0x50;
  return procTable[PROCS - 1].pid;
}

//  executes next available procTable entry (fork of previous) and returns it's PID
int procExec(void *mainFunc)
{
  //  clear ctx
  memset(&procTable[PROCS - 1].ctx, 0, sizeof(procTable[0].ctx));
  //  clear stack to prevent security issues
  memset((uint32_t *)procTable[PROCS - 1].tos - 0x00001000, 0, (uint32_t)0x00001000);
  //  set PC to entrypoint of program
  procTable[PROCS - 1].ctx.pc = (uint32_t)mainFunc;
  procTable[PROCS - 1].ctx.sp = procTable[PROCS - 1].tos;
  procTable[PROCS - 1].priority = 1;
  procTable[PROCS - 1].ctx.cpsr = 0x50;
  return procTable[PROCS - 1].pid;
}

//  delete PID from the procTable and shrink the procTable if it has empty space
void procDelete(pid_t pid)
{
  int position = procTableContains(pid);
  //  if process is in process table
  if (position >= 0)
  {
    //  clear procTable entry
    memset(&procTable[position], 0, sizeof(pcb_t));
    procTable[position].status = STATUS_TERMINATED;
    PROCS--;
    //  defrag process table
    procDefrag();
    if (PROCS <= (procTabSize / 2))
    {
      //  shrink process table if it can be
      procTabSize = procTabSize / 2;
      procTable = realloc(procTable, procTabSize * sizeof(pcb_t));
    }
  }
  return;
}