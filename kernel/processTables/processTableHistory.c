/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */
#include "processTableHistory.h"
#include <stdlib.h>
#include "../../user/console.h"

pcb_t *lastExitProc = NULL;
pcb_t *procTableHistory;
int procTabHistorySize = 0;

//  maximum items that are stored in the history table
int MAX_HISTORY_PROCS = 8;
int HISTORY_PROCS = 0;

/*  makes sure all process table entries are adjacent 
    
    O(n^2)  */
void procHistoryDefrag()
{
  for (int i = 0; i < procTabHistorySize; i++)
  {
    if (procTableHistory[i].pid == 0)
    {
      for (int j = i + 1; j < procTabHistorySize; j++)
      {
        if (procTableHistory[j].pid != 0)
        {
          memcpy(&procTableHistory[i], &procTableHistory[j], sizeof(pcb_t)); // set procTable[i] to procTable[j]
          memset(&procTableHistory[j], 0, sizeof(pcb_t));                    // set procTable[j] to 0
          break;
        }
      }
    }
  }
  return;
}

//  initialise the history table (copy of procTable that stores terminated processes)
//    -dynamically resizing
int procHistoryInit(pcb_t *entry)
{
  if (HISTORY_PROCS >= procTabHistorySize && HISTORY_PROCS < MAX_HISTORY_PROCS && HISTORY_PROCS != 0)
  {
    procTabHistorySize = procTabHistorySize * 2;
    procTableHistory = realloc(procTableHistory, procTabHistorySize * sizeof(pcb_t));
  }
  else if (HISTORY_PROCS == 0)
  {
    procTabHistorySize = 1;
    procTableHistory = malloc(sizeof(pcb_t));
  }

  memcpy(&procTableHistory[HISTORY_PROCS], &entry, sizeof(pcb_t)); // initialise i-th PCB = main
  HISTORY_PROCS++;

  return HISTORY_PROCS;
}

//  deletes an item from the table
void procHistoryDelete(int entryNumber)
{
  if (entryNumber < HISTORY_PROCS && entryNumber < MAX_HISTORY_PROCS)
  {
    memset(&procTableHistory[entryNumber], 0, sizeof(pcb_t));
    HISTORY_PROCS--;
    procHistoryDefrag();

    if (HISTORY_PROCS <= (procTabHistorySize / 2))
    {
      procTabHistorySize = procTabHistorySize / 2;
      procTableHistory = realloc(procTableHistory, procTabHistorySize * sizeof(pcb_t));
    }
  }
  else
  {
    puts("error: entry index out of range\n", 32);
  }
  return;
}
