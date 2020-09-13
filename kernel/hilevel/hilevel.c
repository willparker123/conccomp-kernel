/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */
#include "hilevel.h"
#include "../lolevel.h"
#include "../int.h"
#include "../../user/libc.h"
#include "../processTables/processTable.h"
#include "../processTables/processTableHistory.h"
#include "../scheduling/scheduler.h"
#include "../ipc/shmTable.h"
#include "../ipc/semTable.h"
#include "../../user/console.h"

extern void main_console();
extern uint32_t tos_user;
extern void main_P3();
extern void main_P4();
extern void main_P5();
extern void main_diningPhil();

//  returns single char depending on the status given (for printing)
char statusToString(status_t status)
{
  switch (status)
  {
  case (STATUS_INVALID):
  {
    return 'I';
  }
  case (STATUS_CREATED):
  {
    return 'C';
  }
  case (STATUS_TERMINATED):
  {
    return 'T';
  }
  case (STATUS_READY):
  {
    return 'R';
  }
  case (STATUS_EXECUTING):
  {
    return 'E';
  }
  case (STATUS_WAITING):
  {
    return 'W';
  }
  default:
  {
    return 'I';
  }
  }
}

void hilevel_handler_rst(ctx_t *ctx)
{
  /* Configure the mechanism for interrupt handling by
   *
   * - configuring UART st. an interrupt is raised every time a byte is
   *   subsequently received,
   * - configuring GIC st. the selected interrupts are forwarded to the 
   *   processor via the IRQ interrupt signal, then
   * - enabling IRQ interrupts.
   */
  //  initialise variables
  PROCS_ACTIVE = 0;
  PROCS = 0;
  for (int i = 0; i < procTabSize; i++)
  {
    procTable[i].status = STATUS_INVALID;
  }
  //  invoke and malloc the MLFQ
  invokeQueueMLFQ();

  /* Force execution into an infinite loop, each iteration of which will
   *
   * - delay for some period of time, which is realised by executing a
   *   large, fixed number of nop instructions in an inner loop, then
   * - execute a supervisor call (i.e., svc) instruction, thus raising
   *   a software-interrupt (i.e., a trap or system call).
   */

  //  initialise console in process table
  pid_t pidConsole = procInit(&main_console);
  //  context switch into console
  dispatch(ctx, NULL, &procTable[0]);
  PROCS_ACTIVE++;

  TIMER0->Timer1Load = 0x00100000; // select period = 2^20 ticks ~= 1 sec

  TIMER0->Timer1Ctrl = 0x00000002;  // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer
  //UART0->IMSC       |= 0x00000010; // enable UART    (Rx) interrupt
  //UART0->CR          = 0x00000301; // enable UART (Tx+Rx)
  GICC0->PMR = 0x000000F0;         // unmask all          interrupts
  GICD0->ISENABLER0 |= 0x00000010; // enable UART    (Rx) interrupt
  GICD0->ISENABLER1 |= 0x00000010; // enable UART    (Rx) interrupt
  GICC0->CTLR = 0x00000001;        // enable GIC interface
  GICD0->CTLR = 0x00000001;        // enable GIC distributor

  int_enable_irq();

  return;
}

void hilevel_handler_irq(ctx_t *ctx)
{
  //get interruptor id
  uint32_t id = GICC0->IAR;
  // handle the interrupt, then clear (or reset) the source.
  if (id == GIC_SOURCE_UART0)
  {
    uint8_t x = PL011_getc(UART0, true);

    PL011_putc(UART0, 'K', true);
    PL011_putc(UART0, '<', true);
    PL011_putc(UART0, itox((x >> 4) & 0xF), true);
    PL011_putc(UART0, itox((x >> 0) & 0xF), true);
    PL011_putc(UART0, '>', true);
  }

  if (id == GIC_SOURCE_TIMER0)
  {
    //  schedule every tick
    PL011_putc(UART0, 'T', true);
    TIMER0->Timer1IntClr = 0x01;
    schedule(ctx);
  }

  //signal to interruptor IRQ has been handled
  GICC0->EOIR = id;

  return;
}

void hilevel_handler_svc(ctx_t *ctx, uint32_t id)
{
  switch (id)
  {
  case 0x00:
  { // 0x00 => yield( pid )
    pid_t pid = (pid_t)(ctx->gpr[0]);
    //  context switch into yielding process
    dispatch(ctx, currentProc, &procTable[pid]);
    break;
  }

  case 0x01:
  { // 0x01 => write( fd, x, n )
    int fd = (int)(ctx->gpr[0]);
    char *x = (char *)(ctx->gpr[1]);
    int n = (int)(ctx->gpr[2]);

    for (int i = 0; i < n; i++)
    {
      PL011_putc(UART0, *x++, true);
    }

    ctx->gpr[0] = n;

    break;
  }

  case 0x02:
  { // 0x02 => read( fd, x, n )
    int fd = (int)(ctx->gpr[0]);
    char *x = (char *)(ctx->gpr[1]);
    int n = (int)(ctx->gpr[2]);

    for (int i = 0; i < n; i++)
    {
      PL011_getc(UART0, true);
    }

    ctx->gpr[0] = n;

    break;
  }

  case 0x03:
  { // 0x03 => fork( )
    if (PROCS < MAX_PROCS)
    {
      //  disable scheduling
      //disableMLFQ();
      currentProc->status = STATUS_READY;
      memcpy(&currentProc->ctx, ctx, sizeof(ctx_t));
      pid_t pidChild = procCopy(currentProc, ctx);
      currentProc->ctx.gpr[0] = pidChild;
      dispatch(ctx, currentProc, &procTable[procTableContains(pidChild)]);
      PROCS_ACTIVE++;
    }
    else
    {
      //  TODO MMU
      puts("error: exceeded MAX_PROCS\n", 26);
      //  return -1 as error value
      ctx->gpr[0] = -1;
    }

    break;
  }

  case 0x04:
  { // 0x04 => exit(x)
    pid_t pid = currentProc->pid;
    int exitStatus = (int)(ctx->gpr[0]);

    if (procTableContains(pid) > 0)
    {
      if (exitStatus == EXIT_SUCCESS)
      {
        semTableRemove(pid);
        shmTabRemove(pid);
        //  remove process from queue and delete process table entry and reschedule next process
        removeFromMLFQ(pid);
        procDelete(pid);
        schedule(ctx);
      }
      else
      {
        semTableRemove(pid);
        shmTabRemove(pid);
        //  same as above but store in history table
        procHistoryInit(&procTable[procTableContains(pid)]);
        removeFromMLFQ(pid);
        procDelete(pid);
        puts("console$ process killed and stored in history table\n", 52);
        schedule(ctx);
      }
      PROCS_ACTIVE--;
    }
    else
    {
      puts("error: process not active\n", 26);
    }
    break;
  }

  case 0x05:
  { // 0x05 => exec( addr )
    if (PROCS < MAX_PROCS)
    {
      void *addr = (void *)(ctx->gpr[0]);
      //  reset stack of fork,
      pid_t pid = procExec(addr);
      dispatch(ctx, NULL, &procTable[procTableContains(pid)]);

      //  enableMLFQ();
    }
    else
    {
      //  TODO MMU
      puts("error: exceeded MAX_PROCS\n", 26);
    }
    break;
  }

  case 0x06:
  { // 0x06 => kill( pid, x )
    pid_t pid = (pid_t)(ctx->gpr[0]);
    int exitStatus = (int)(ctx->gpr[1]);

    if (procTableContains(pid) > 0)
    {
      if (exitStatus == EXIT_SUCCESS)
      {
        semTableRemove(pid);
        shmTabRemove(pid);
        removeFromMLFQ(pid);
        procDelete(pid);
        schedule(ctx);
      }
      else
      {
        procHistoryInit(&procTable[procTableContains(pid)]);
        semTableRemove(pid);
        shmTabRemove(pid);
        removeFromMLFQ(pid);
        procDelete(pid);
        puts("console$ process killed and stored in history table\n", 52);
        schedule(ctx);
      }
      PROCS_ACTIVE--;
      ctx->gpr[0] = 0;
    }
    else
    {
      puts("error: process not active\n", 26);
      ctx->gpr[0] = -1;
    }
    break;
  }

  case 0x07:
  { // 0x07 => prio( pid, p )
    pid_t pid = (pid_t)(ctx->gpr[0]);
    int p = (int)(ctx->gpr[1]);
    char *pidString;

    if (procTableContains(pid) > -1)
    {
      if (p > 0)
      {
        procTable[procTableContains(pid)].priority = p;
        schedule(ctx);
      }
      else
      {
        puts("error: priority must be >= 0\n", 29);
      }
    }
    else
    {
      puts("error: no process with id ", 26);
      itoaLocal(pidString, pid);
      puts(pidString, strlen(pidString));
      puts("\n", 1);
    }
    break;
  }

  case 0x08:
  { // 0x08 => pause( pid, x )
    pid_t pid = (pid_t)(ctx->gpr[0]);

    if (procTable[procTableContains(pid)].status != STATUS_WAITING)
    {
      if (procTableContains(pid) > 0)
      {
        PROCS_ACTIVE--;
        procTable[procTableContains(pid)].priority = 0;
        procTable[procTableContains(pid)].status = STATUS_WAITING;
        removeFromMLFQ(pid);
      }
      else
      {
        puts("error: process not active\n", 26);
      }
    }
    else
    {
      puts("error: process already paused\n", 30);
    }
    break;
  }

  case 0x09:
  { // 0x09 => unpause( pid )
    pid_t pid = (pid_t)(ctx->gpr[0]);

    if (procTableContains(pid) > 0 && procTable[procTableContains(pid)].status == STATUS_WAITING)
    {
      procTable[procTableContains(pid)].status = STATUS_READY;
      procTable[procTableContains(pid)].priority = 1;
      PROCS_ACTIVE++;
      schedule(ctx);
    }
    else
    {
      puts("error: process not paused\n", 26);
    }
    break;
  }

  case 0x10:
  { // 0x10 => status()
    puts("---PROCESS TABLE ENTRIES:---\n", 29);
    if (PROCS > 1)
    {
      for (int i = 0; i < PROCS; i++)
      {
        if (procTable[i].pid != -1)
        {
          pid_t pid = procTable[i].pid;
          status_t status = procTable[i].status;
          int priority = procTable[i].priority;
          char *pidString;
          char *priorityString;
          char *statusString = malloc(sizeof(char));
          statusString[0] = statusToString(status);

          puts("---ID: ", 7);
          itoaLocal(pidString, pid);
          puts(pidString, strlen(pidString));
          puts("\n", 1);
          puts("---PRIORITY: ", 13);
          itoaLocal(priorityString, priority);
          puts(priorityString, strlen(priorityString));
          puts("\n", 1);
          puts("---STATUS: ", 11);
          puts(statusString, strlen(statusString));
          puts("\n", 1);
          puts("----------------------------\n", 29);
        }
      }
    }
    else
    {
      puts("---No entries\n", 14);
      puts("----------------------------\n", 29);
    }
    break;
  }

  case 0x11:
  { // 0x11 => history()
    puts("---PROCESS TABLE HISTORY:---\n", 29);
    if (HISTORY_PROCS > 0)
    {
      for (int i = 0; i < HISTORY_PROCS; i++)
      {
        pid_t pid = procTableHistory[i].pid;
        status_t status = procTableHistory[i].status;
        int priority = procTableHistory[i].priority;
        char *pidString;
        char *priorityString;
        char *statusString = malloc(sizeof(char));
        statusString[0] = statusToString(status);
        itoaLocal(pidString, pid);
        itoaLocal(priorityString, priority);

        puts("---ID: ", 7);
        puts(pidString, strlen(pidString));
        puts("\n", 1);
        puts("---PRIORITY: ", 13);
        puts(priorityString, strlen(priorityString));
        puts("\n", 1);
        puts("---STATUS: ", 11);
        puts(statusString, strlen(statusString));
        puts("\n", 1);
        puts("----------------------------\n", 29);
      }
    }
    else
    {
      puts("---No entries\n", 14);
      puts("----------------------------\n", 29);
    }
    break;
  }

  case 0x12:
  { // 0x12 => close(x)
    int exitStatus = (int)(ctx->gpr[1]);

    for (int i = (PROCS - 1); i < 0; i--)
    {
      semTableRemove(procTable[i].pid);
      shmTabRemove(procTable[i].pid);
      removeFromMLFQ(procTable[i].pid);
      procDelete(procTable[i].pid);
    }
    for (int j = 0; j < HISTORY_PROCS; j++)
    {
      procHistoryDelete(procTableHistory[j].pid);
    }
    free(procTable);
    free(procTableHistory);
    deleteMLFQ();
    if (exitStatus == EXIT_FAILURE)
    {
      puts("SYSTEM CRASH\n", 13);
    }
    else
    {
      puts("SYSTEM CLOSED\n", 14);
    }
  }

  case 0x13:
  { // 0x13 => forkproc(pid)
    pid_t pid = (pid_t)(ctx->gpr[0]);
    int proc = procTableContains(pid);
    if (proc > -1)
    {
      if (PROCS < MAX_PROCS)
      {
        pid_t pidChild = procCopy(currentProc, ctx);
        currentProc->status = STATUS_READY;
        currentProc->ctx.gpr[0] = pidChild;
        dispatch(ctx, currentProc, &procTable[procTableContains(pidChild)]);
        PROCS_ACTIVE++;
      }
      else
      {
        //  TODO MMU
        puts("error: exceeded MAX_PROCS\n", 26);

        ctx->gpr[0] = -1;
      }
    }
    else
    {
      puts("error: invalid PID\n", 19);

      ctx->gpr[0] = -1;
    }

    break;
  }

  case 0x14:
  { // 0x14 => getaddr()
    uint32_t pc = (uint32_t)(ctx->pc);
    ctx->gpr[0] = pc;
    break;
  }

  //  ----IPC----
  case 0x20:
  { // 0x20 => shm_init( size )
    size_t size = (size_t)(ctx->gpr[0]);
    pid_t owner = currentProc->pid;

    int index = shmTabInit(owner, size);

    puts("console$ shared memory segment initialised\n", 43);

    ctx->gpr[0] = (uint32_t)shmTable[index].addr;

    break;
  }

  case 0x21:
  { // 0x21 => shm_destroy( addr, pid )
    //if pid = owner remove from shmTable, free and return 0. Else return -1
    void *addr = (void *)(ctx->gpr[0]);
    pid_t pid = currentProc->pid;

    int index = shmTabContains(addr);

    if (index > -1)
    {
      if (shmTable[index].owner == pid)
      {
        shmTabDelete(addr);
        free(addr);
      }
      else
      {
        puts("error: shared memory belongs to parent process\n", 47);
      }
    }
    else
    {
      puts("error: shared memory segment not found\n", 39);
    }
    break;
  }

  case 0x22:
  { // 0x22 => shm_write( addr, data, dataSize )
    int *addr = (int *)(ctx->gpr[0]);
    int data = (int)(ctx->gpr[1]);
    size_t size = (size_t)(ctx->gpr[2]);

    int index = shmTabContains(addr);

    if (index > -1)
    {
      if (size <= shmTable[index].size)
      {
        memcpy(addr, &data, size);
        puts("wrote to shared address ", 24);
        puts("[", 1);
        char *addrString;
        itoaLocal(addrString, (uint32_t)addr);
        puts(addrString, 8);
        puts("]\n", 2);
      }
      else
      {
        puts("error: new data is too big for shared memory segment\n", 53);
      }
    }
    else
    {
      puts("error: shared memory segment not found\n", 39);
    }
    break;
  }

  case 0x30:
  { // 0x30 => sem_init()
    sem_t sem = (int *)malloc(sizeof(int));
    *sem = 0;

    semTableAdd(sem, 0, currentProc->pid);

    puts("console$ semaphore initialised\n", 31);

    ctx->gpr[0] = (uint32_t)sem;

    break;
  }

  case 0x31:
  { // 0x31 => sem_destroy( sem )
    sem_t sem = (sem_t)(ctx->gpr[0]);

    int index = semTabContains(sem);

    if (*sem == 0)
    {
      if (semTable[index].owner == currentProc->pid)
      {
        semTableRemove(currentProc->pid);
        puts("console$ semaphore destoyed\n", 28);
      }
      else
      {
        puts("error: semaphore belongs to parent process\n", 43);
      }
    }
    else
    {
      puts("error: semaphore value not 0\n", 29);
    }

    break;
  }

  case 0x32:
  { // 0x32 => sem_post( sem )
    sem_t sem = (sem_t)(ctx->gpr[0]);

    int s = *sem;
    if (s == *sem)
    {
      *sem++;
      semTableNotify(sem);
      puts("semaphore post ", 15);
      puts("[", 1);
      char *string;
      itoaLocal(string, (uint32_t)sem);
      puts(string, 8);
      puts("]\n", 2);
    }
    else
    {
      puts("error: race condition on semaphore\n", 35);
    }

    break;
  }

  case 0x33:
  { // 0x33 => sem_wait( sem )
    sem_t sem = (sem_t)(ctx->gpr[0]);

    if (*sem > 0)
    {
      *sem--;

      ctx->gpr[0] = *sem;
      break;
    }

    if (*sem < 0)
    {
      puts("error: semaphore value < 0\n", 27);
      ctx->gpr[0] = -1;
      break;
    }

    procTable[procTableContains(currentProc->pid)].status = STATUS_WAITING;
    semTableAdd(sem, currentProc->pid, semGetOwner(sem));

    puts("semaphore wait ", 15);
    puts("[", 1);
    char *string;
    itoaLocal(string, (uint32_t)sem);
    puts(string, 8);
    puts("]\n", 2);

    ctx->gpr[0] = 0;
    break;
  }

  default:
  { // 0x?? => unknown/unsupported
    break;
  }
  }
  return;
}
