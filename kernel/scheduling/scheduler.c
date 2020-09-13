#include "scheduler.h"
#include <stdlib.h>

//  maximum levels in mlfq - excludes Round Robin (0 implies Round Robin only)
int MAX_QUEUE_LEVELS = 4;

//  multi level FCFS queue with level 0 = round robin
mlfq_t *multiLevelQueue;

//  disable scheduling (when a fork() is occuring)
void disableMLFQ()
{
  multiLevelQueue->enabled = false;
}

//  enable scheduling (after a fork())
void enableMLFQ()
{
  multiLevelQueue->enabled = true;
}

//  power function for calculating "t" of each level in queue
int pow(int base, int exp)
{
  if (exp < 0)
  {
    return -1;
  }
  int result = 1;
  while (exp)
  {
    if (exp & 1)
      result *= base;
    exp >>= 1;
    base *= base;
  }
  return result;
}

//  create round robin queue (id = 0, t = 1)
void invokeQueueRR()
{
  multiLevelQueue->queueRoundRobin = malloc(sizeof(queue_t));
  multiLevelQueue->queueRoundRobin->id = 0;
  multiLevelQueue->queueRoundRobin->t = 1;
  multiLevelQueue->queueRoundRobin->processes = malloc(sizeof(pid_t));
  multiLevelQueue->queueRoundRobin->length = 0;
  multiLevelQueue->queueRoundRobin->size = 1;
  return;
}

//  create a FCFS queue
void invokeQueueFCFS()
{
  //  get current number of FCFS queues in mlfq
  int levels = multiLevelQueue->levels;
  //  create first FCFS (t = 1,2,4,8, id = 1)
  if (levels == 0)
  {
    multiLevelQueue->queuesFCFS = malloc((levels + 1) * sizeof(queue_t));
    multiLevelQueue->queuesFCFS[levels].id = levels + 1;
    multiLevelQueue->queuesFCFS[levels].t = pow(2, levels);
    multiLevelQueue->queuesFCFS[levels].processes = malloc(sizeof(pid_t));
    multiLevelQueue->queuesFCFS[levels].length = 0;
    multiLevelQueue->queuesFCFS[levels].size = 1;
  }
  else
  {
    //  add an FCFS to the mlfq with t = 1,2,4,8...
    multiLevelQueue->queuesFCFS = realloc(multiLevelQueue->queuesFCFS, (levels + 1) * sizeof(queue_t));
    multiLevelQueue->queuesFCFS[levels].processes = calloc(1, sizeof(pid_t));
    multiLevelQueue->queuesFCFS[levels].id = levels + 1;
    multiLevelQueue->queuesFCFS[levels].t = pow(2, levels);
    multiLevelQueue->queuesFCFS[levels].length = 0;
    multiLevelQueue->queuesFCFS[levels].size = 1;
  }
  multiLevelQueue->levels++;
  return;
}

//  creates the MLFQ object
void invokeQueueMLFQ()
{
  multiLevelQueue = malloc(sizeof(mlfq_t));
  multiLevelQueue->levels = 0;
  multiLevelQueue->processCount = 0;
  invokeQueueFCFS();
  invokeQueueRR();
  enableMLFQ();
  return;
}

//  deletes and frees the MLFQ
void deleteMLFQ()
{
  free(multiLevelQueue->queueRoundRobin);
  for (int i = 0; i < multiLevelQueue->levels; i++)
  {
    free(&multiLevelQueue->queuesFCFS[i]);
  }
  free(multiLevelQueue);
  return;
}

//  returns a qpos_t: {level: int, index: int} (position) of a PID in the queue. {-1, -1} if PID not in the MLFQ
qpos_t inMLFQ(pid_t pid)
{
  //  initialise position as "not in MLFQ"
  qpos_t position;
  position.level = -1;
  position.index = -1;
  //  for all FCFS queues in the MLFQ
  for (int level = 0; level <= multiLevelQueue->levels; level++)
  {
    //  for every process in the FCFS queue
    if (level == 0)
    {
      for (int index = 0; index < multiLevelQueue->queueRoundRobin->size; index++)
      {
        if (multiLevelQueue->queueRoundRobin->processes[index] == pid)
        {
          position.level = level;
          position.index = index;
          return position;
        }
      }
    }
    //  for every process in the round robin queue
    else
    {
      for (int index = 0; index < multiLevelQueue->queuesFCFS[level - 1].size; index++)
      {
        if (multiLevelQueue->queuesFCFS[level - 1].processes[index] == pid)
        {
          position.level = level;
          position.index = index;
          return position;
        }
      }
    }
  }
  return position;
}

/*  makes sure all queue entries are adjacent
    
    O(n^2)  */
void queueDefrag(int level)
{
  if (level > 0)
  {
    int target = level - 1;
    for (int i = 0; i < multiLevelQueue->queuesFCFS[target].size; i++)
    {
      //  if the item at position i in the queue is empty
      if (multiLevelQueue->queuesFCFS[target].processes[i] == 0)
      {
        //  look through the rest of the processes in the queue
        for (int j = i + 1; j < multiLevelQueue->queuesFCFS[target].size; j++)
        {
          //  if a larger index process is not empty, swap with earlier empty slot
          if (multiLevelQueue->queuesFCFS[target].processes[j] != 0)
          {
            multiLevelQueue->queuesFCFS[target].processes[i] = multiLevelQueue->queuesFCFS[target].processes[j];
            multiLevelQueue->queuesFCFS[target].processes[j] = 0;
            break;
          }
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < multiLevelQueue->queueRoundRobin->size; i++)
    {
      //  if the item at position i in the queue is empty
      if (multiLevelQueue->queueRoundRobin->processes[i] == 0)
      {
        //  look through the rest of the processes in the queue
        for (int j = i + 1; j < multiLevelQueue->queueRoundRobin->size; j++)
        {
          //  if a larger index process is not empty, swap with earlier empty slot
          if (multiLevelQueue->queueRoundRobin->processes[j] != 0)
          {
            multiLevelQueue->queueRoundRobin->processes[i] = multiLevelQueue->queueRoundRobin->processes[j]; // set procTable[i] to procTable[j]
            multiLevelQueue->queueRoundRobin->processes[j] = 0;
            break;
          }
        }
      }
    }
  }
  return;
}

//  adds a PID to a given level in the MLFQ (level 0 = round robin)
void addToQueue(pid_t pid, int level)
{
  if (level == 0)
  {
    //  if queue has room
    if (multiLevelQueue->queueRoundRobin->length < multiLevelQueue->queueRoundRobin->size)
    {
      //  add pid to MLFQ
      multiLevelQueue->queueRoundRobin->processes[multiLevelQueue->queueRoundRobin->length] = pid;
      multiLevelQueue->queueRoundRobin->length++;
    }
    else
    {
      //  double size of queue
      multiLevelQueue->queueRoundRobin->size = multiLevelQueue->queueRoundRobin->size * 2;
      multiLevelQueue->queueRoundRobin->processes = realloc(multiLevelQueue->queueRoundRobin->processes, multiLevelQueue->queueRoundRobin->size * sizeof(pid_t));
      multiLevelQueue->queueRoundRobin->processes[multiLevelQueue->queueRoundRobin->length] = pid;
      multiLevelQueue->queueRoundRobin->length++;
    }
    //  make sure the processes are at the start of the array
    queueDefrag(0);
  }
  else
  {
    //  if the queue has room
    if (multiLevelQueue->queuesFCFS[level - 1].length < multiLevelQueue->queuesFCFS[level - 1].size)
    {
      //  add pid to MLFQ
      multiLevelQueue->queuesFCFS[level - 1].processes[multiLevelQueue->queuesFCFS[level - 1].length] = pid;
      multiLevelQueue->queuesFCFS[level - 1].length++;
    }
    else
    {
      //  double size of queue
      multiLevelQueue->queuesFCFS[level - 1].size = multiLevelQueue->queuesFCFS[level - 1].size * 2;
      multiLevelQueue->queuesFCFS[level - 1].processes = realloc(multiLevelQueue->queuesFCFS[level - 1].processes, multiLevelQueue->queuesFCFS[level - 1].size * sizeof(pid_t));
      multiLevelQueue->queuesFCFS[level - 1].processes[multiLevelQueue->queuesFCFS[level - 1].length] = pid;
      multiLevelQueue->queuesFCFS[level - 1].length++;
    }
    //  make sure the processes are at the start of the array
    queueDefrag(level);
  }
  multiLevelQueue->processCount++;
  return;
}

//  add a PID to the MLFQ
void addToMLFQ(pid_t pid)
{
  //  if pid is not in the MLFQ
  if (inMLFQ(pid).level < 0)
  {
    //  if a process can be added to the scheduler
    if (PROCS < MAX_PROCS)
    {
      //  add to top-level queue
      addToQueue(pid, 1);
    }
    else
    {
      //  TODO MMU
    }
  }
  return;
}

//  remove a PID from the MLFQ
void removeFromMLFQ(pid_t pid)
{
  qpos_t position = inMLFQ(pid);
  //  if pid is in the MLFQ
  if (position.level >= 0)
  {
    //  set process id to 0 (no process can have pid=0; empty the space in the array)
    if (position.level == 0)
    {
      multiLevelQueue->queueRoundRobin->processes[position.index] = 0;
      multiLevelQueue->queueRoundRobin->length--;
    }
    else
    {
      multiLevelQueue->queuesFCFS[position.level - 1].processes[position.index] = 0;
      multiLevelQueue->queuesFCFS[position.level - 1].length--;
    }

    //  defrag the queue
    queueDefrag(position.level);

    if (position.level == 0)
    {
      //  if the queue can be resized, do so
      if (multiLevelQueue->queueRoundRobin->length <= (multiLevelQueue->queueRoundRobin->size / 2) && multiLevelQueue->queueRoundRobin->size > 1)
      {
        //  half size of queue
        multiLevelQueue->queueRoundRobin->size = multiLevelQueue->queueRoundRobin->size / 2;
        multiLevelQueue->queueRoundRobin->processes = realloc(multiLevelQueue->queueRoundRobin->processes, multiLevelQueue->queueRoundRobin->size * sizeof(pid_t));
      }
    }
    else
    {
      //  if the queue can be resized, do so
      if (multiLevelQueue->queuesFCFS[position.level - 1].length <= (multiLevelQueue->queuesFCFS[position.level - 1].size / 2) && multiLevelQueue->queuesFCFS[position.level - 1].size > 1)
      {
        //  half size of queue
        multiLevelQueue->queuesFCFS[position.level - 1].size = multiLevelQueue->queuesFCFS[position.level - 1].size / 2;
        multiLevelQueue->queuesFCFS[position.level - 1].processes = realloc(multiLevelQueue->queuesFCFS[position.level - 1].processes, multiLevelQueue->queuesFCFS[position.level - 1].size * sizeof(pid_t));
      }

      //  if the queue can be deleted, do so
      //  if the queue being deleted is the lowest level queue or lower
      if (position.level >= multiLevelQueue->levels && multiLevelQueue->queuesFCFS[position.level - 1].length <= 0 && multiLevelQueue->levels > 1)
      {
        int levels = multiLevelQueue->levels;
        memset(&multiLevelQueue->queuesFCFS[levels - 1], 0, sizeof(queue_t));
        multiLevelQueue->queuesFCFS = realloc(multiLevelQueue->queuesFCFS, (levels - 1) * sizeof(queue_t));
        multiLevelQueue->levels--;
      }
    }
    multiLevelQueue->processCount--;
  }
  return;
}

//  dispatch a process into the current context
void dispatch(ctx_t *ctx, pcb_t *prev, pcb_t *next)
{
  char prev_pid = '?', next_pid = '?';

  if (NULL != prev)
  {
    memcpy(&prev->ctx, ctx, sizeof(ctx_t)); // preserve execution context of P_{prev} [TO; FROM; SIZE]
    prev_pid = '0' + prev->pid;
  }
  if (NULL != next)
  {
    memcpy(ctx, &next->ctx, sizeof(ctx_t)); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

  PL011_putc(UART0, '[', true);
  PL011_putc(UART0, prev_pid, true);
  PL011_putc(UART0, '-', true);
  PL011_putc(UART0, '>', true);
  PL011_putc(UART0, next_pid, true);
  PL011_putc(UART0, ']', true);
  prev->status = STATUS_READY;

  //  if the process is not in the MLFQ, initialise it in the MLFQ
  if (inMLFQ(next->pid).level < 0)
  {
    addToMLFQ(next->pid);
  }

  currentProc = next; // update executing process to P_{next}
  currentProc->status = STATUS_EXECUTING;

  return;
}

/*  MLFQ priority-based scheduler with [MAX_QUEUE_LEVELS] FCFS queues and a bottom-level (id/level = 0) Round Robin queue.
      -dynamically resizing queues of size 2^n on add/remove
      -dynamically resizing queue levels (unnessecary queues are removed, more are added if needed up to MAX_QUEUE_LEVELS)
      -pausing mechanism (status = STATUS_WAITING or priority = 0)
      -priority increases by 1 every tick
      -processes moved to lower level after priority doubles
*/
void schedule(ctx_t *ctx)
{ //  check for fork()
  if (multiLevelQueue->enabled)
  {
    //  if there is multiprocessing
    if (PROCS > 1)
    {
      //  loop over all process in procTable
      for (int i = 0; i < PROCS; i++)
      {
        //  if STATUS_INVALID, try restart the process
        switch (procTable[i].status)
        {
        case (STATUS_INVALID):
        {
          uint32_t iprocTos = procTable[i].tos;
          procDelete(procTable[i].pid);
          pid_t iprocPid = procInit(&iprocTos);
          break;
        }
        default:
        {
          //  if the processes are all not in the round robin, increment priority
          if (multiLevelQueue->queueRoundRobin->length != PROCS)
          {
            procTable[i].priority = procTable[i].priority + 1;
          }
          //  if the priority is larger than the "t" of the last level FCFS queue, add to round robin
          if (procTable[i].priority > pow(2, MAX_QUEUE_LEVELS - 1))
          {
            if (procTable[i].priority == pow(2, MAX_QUEUE_LEVELS - 1) + 1)
            {
              //  if process has taken longer than the "t" of the final FCFS queue
              removeFromMLFQ(procTable[i].pid);
              //  add to round robin
              addToQueue(procTable[i].pid, 0);
            }
          }
          else if (procTable[i].priority == 0)
          {
            removeFromMLFQ(procTable[i].pid);
          }
          else
          {
            //  shuffle process into new queue (if needed) based on priority and "t" of the queue at level l
            for (int l = 0; l < MAX_QUEUE_LEVELS; l++)
            {
              if (procTable[i].priority == pow(2, l))
              {
                if (procTable[i].priority == pow(2, multiLevelQueue->levels))
                {
                  invokeQueueFCFS();
                }
                removeFromMLFQ(procTable[i].pid);
                addToQueue(procTable[i].pid, l + 1);
              }
            }
          }
          break;
        }
        }
      }

      //  if the processes are not all in the round robin
      if (multiLevelQueue->queueRoundRobin->length < PROCS)
      {
        //  loop over all FCFS queues
        for (int x = 0; x < multiLevelQueue->levels; x++)
        {
          PL011_putc(UART0, 'x', true);
          //  loop over all items in the queue
          for (int y = 0; y < multiLevelQueue->queuesFCFS[x].length; y++)
          {
            PL011_putc(UART0, 'y', true);
            //  if the process in this queue  and if process priority != 0 and has status != STATUS_WAITING and is not the current process
            if (multiLevelQueue->queuesFCFS[x].processes[y] != currentProc->pid &&
                multiLevelQueue->queuesFCFS[x].processes[y] != 0 && procTable[procTableContains(multiLevelQueue->queuesFCFS[x].processes[y])].status != STATUS_WAITING && procTable[procTableContains(multiLevelQueue->queuesFCFS[x].processes[y])].priority != 0)
            {

              //  context switch the "next-to-be-scheduled" process into the current process
              dispatch(ctx, currentProc, &procTable[procTableContains(multiLevelQueue->queuesFCFS[x].processes[y])]); // context switch P_1 -> P_2
              return;
            }
          }
        }

        //  for all processes in the round robin
        for (int z = 0; z < multiLevelQueue->queueRoundRobin->length; z++)
        {
          //  find currentProc and if process priority != 0 and has status != STATUS_WAITING
          if (multiLevelQueue->queueRoundRobin->processes[z] == currentProc->pid &&
              multiLevelQueue->queueRoundRobin->processes[z] != 0 && procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[z])].status != STATUS_WAITING && procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[z])].priority != 0)
          {
            //  execute next item in the round robin
            if (z != multiLevelQueue->queueRoundRobin->length - 1)
            {
              dispatch(ctx, currentProc, &procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[z + 1])]); // context switch P_1 -> P_2
              return;
            }
            else
            {
              //context switch the first available process in the Round Robin queue into the current process
              dispatch(ctx, currentProc, &procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[0])]); // context switch P_1 -> P_2
              return;
            }
          }
        }
      }
      else
      {
        //  for all processes in the round robin
        for (int z = 0; z < multiLevelQueue->queueRoundRobin->size; z++)
        {
          //  find currentProc and if process priority != 0 and has status != STATUS_WAITING
          if (multiLevelQueue->queueRoundRobin->processes[z] == currentProc->pid && multiLevelQueue->queueRoundRobin->processes[z] != 0 && procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[z])].status != STATUS_WAITING && procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[z])].priority != 0)
          {
            //  execute next item in the round robin
            if (z != multiLevelQueue->queueRoundRobin->length - 1)
            {
              dispatch(ctx, currentProc, &procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[z + 1])]); // context switch P_1 -> P_2
              return;
            }
            else
            {
              //context switch the first available process in the Round Robin queue into the current process
              dispatch(ctx, currentProc, &procTable[procTableContains(multiLevelQueue->queueRoundRobin->processes[0])]); // context switch P_1 -> P_2
              return;
            }
          }
        }
      }
    }
  }
}
