/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

int atoiLocal(char *x)
{
  char *p = x;
  bool s = false;
  int r = 0;

  if (*p == '-')
  {
    s = true;
    p++;
  }
  else if (*p == '+')
  {
    s = false;
    p++;
  }

  for (int i = 0; *p != '\x00'; i++, p++)
  {
    r = s ? (r * 10) - (*p - '0') : (r * 10) + (*p - '0');
  }

  return r;
}

void itoaLocal(char *r, int x)
{
  char *p = r;
  int t, n;

  if (x < 0)
  {
    p++;
    t = -x;
    n = t;
  }
  else
  {
    t = +x;
    n = t;
  }

  do
  {
    p++;
    n /= 10;
  } while (n);

  *p-- = '\x00';

  do
  {
    *p-- = '0' + (t % 10);
    t /= 10;
  } while (t);

  if (x < 0)
  {
    *p-- = '-';
  }

  return;
}

void yield(pid_t pid)
{
  asm volatile("mov r0, %1 \n" // assign r0 = pid
               "svc %0     \n" // make system call SYS_YIELD
               :
               : "I"(SYS_YIELD), "r"(pid)
               : "r0");

  return;
}

int write(int fd, const void *x, size_t n)
{
  int r;

  asm volatile("mov r0, %2 \n" // assign r0 = fd
               "mov r1, %3 \n" // assign r1 =  x
               "mov r2, %4 \n" // assign r2 =  n
               "svc %1     \n" // make system call SYS_WRITE
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_WRITE), "r"(fd), "r"(x), "r"(n)
               : "r0", "r1", "r2");

  return r;
}

int read(int fd, void *x, size_t n)
{
  int r;

  asm volatile("mov r0, %2 \n" // assign r0 = fd
               "mov r1, %3 \n" // assign r1 =  x
               "mov r2, %4 \n" // assign r2 =  n
               "svc %1     \n" // make system call SYS_READ
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_READ), "r"(fd), "r"(x), "r"(n)
               : "r0", "r1", "r2");

  return r;
}

int fork()
{
  int r;

  asm volatile("svc %1     \n" // make system call SYS_FORK
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_FORK)
               : "r0");

  return r;
}

void exit(int x)
{
  asm volatile("mov r0, %1 \n" // assign r0 =  x
               "svc %0     \n" // make system call SYS_EXIT
               :
               : "I"(SYS_EXIT), "r"(x)
               : "r0");

  return;
}

void close(int x)
{
  asm volatile("mov r0, %1 \n" // assign r0 =  x
               "svc %0     \n" // make system call SYS_EXIT
               :
               : "I"(SYS_CLOSE), "r"(x)
               : "r0");

  return;
}

void status()
{
  asm volatile("svc %0     \n" // make system call SYS_EXIT
               :
               : "I"(SYS_STATUS));

  return;
}

void history()
{
  asm volatile("svc %0     \n" // make system call SYS_EXIT
               :
               : "I"(SYS_HISTORY));

  return;
}

void exec(const void *x)
{
  asm volatile("mov r0, %1 \n" // assign r0 = x
               "svc %0     \n" // make system call SYS_EXEC
               :
               : "I"(SYS_EXEC), "r"(x)
               : "r0");

  return;
}

int kill(pid_t pid, int x)
{
  int r;

  asm volatile("mov r0, %2 \n" // assign r0 =  pid
               "mov r1, %3 \n" // assign r1 =    x
               "svc %1     \n" // make system call SYS_KILL
               "mov %0, r0 \n" // assign r0 =    r
               : "=r"(r)
               : "I"(SYS_KILL), "r"(pid), "r"(x)
               : "r0", "r1");

  return r;
}

void nice(pid_t pid, int p)
{
  asm volatile("mov r0, %1 \n" // assign r0 =  pid
               "mov r1, %2 \n" // assign r1 =    x
               "svc %0     \n" // make system call SYS_PRIO
               :
               : "I"(SYS_PRIO), "r"(pid), "r"(p)
               : "r0", "r1");

  return;
}

void pause(pid_t pid)
{
  asm volatile("mov r0, %1 \n"
               "svc %0     \n"
               :
               : "I"(SYS_PAUSE), "r"(pid)
               : "r0");

  return;
}

void unpause(pid_t pid)
{
  asm volatile("mov r0, %1 \n"
               "svc %0     \n"
               :
               : "I"(SYS_UNPAUSE), "r"(pid)
               : "r0");

  return;
}

int forkProc(pid_t pid)
{
  int r;

  asm volatile("mov %0, r0 \n" // assign r0 =  pid
               "svc %1     \n" // make system call SYS_KILL
               "mov r0, %1 \n"
               : "=r"(r)
               : "I"(SYS_FORKPROC), "r"(pid)
               : "r0");
  return r;
}

int getaddr()
{
  int r;

  asm volatile("svc %1     \n" // make system call SYS_FORK
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_GETADDR)
               : "r0");

  return r;
}

//  initialise an empty (0) semaphore
sem_t sem_init()
{
  sem_t r;

  asm volatile("svc %1     \n" // make system call SYS_READ
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_SEM_INIT)
               : "r0");

  return r;
}

//  destroys (frees) semaphore
void sem_destroy(sem_t sem)
{
  asm volatile("mov r0, %1 \n" // assign r0 = x
               "svc %0     \n" // make system call SYS_EXEC
               :
               : "I"(SYS_SEM_DESTROY), "r"(sem)
               : "r0");

  return;
}

//  post to semaphore (increment by 1)
void sem_post(sem_t sem)
{
  asm volatile("mov r0, %1 \n" // assign r0 = x
               "svc %0     \n" // make system call SYS_EXEC
               :
               : "I"(SYS_SEM_POST), "r"(*sem)
               : "r0");

  return;
}

//  same as POSIX sem_wait (decrement if >0 and return *sem, if <0 return -1, if *sem = 0 return 0)
int sem_wait(sem_t sem)
{
  int r;

  asm volatile("mov r0, %2 \n" // assign r0 = fd
               "svc %1     \n" // make system call SYS_READ
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_SEM_WAIT), "r"(sem)
               : "r0");

  while (r == 0)
  {
    r = *sem;
  }
  return r;
}

//  allocate shared memory space of given size
void *shm_init(size_t size)
{
  void *r;

  asm volatile("mov r0, %2 \n" // assign r0 = fd
               "svc %1     \n" // make system call SYS_READ
               "mov %0, r0 \n" // assign r  = r0
               : "=r"(r)
               : "I"(SYS_SHM_INIT), "r"(size)
               : "r0");

  return r;
}

//  free shared memory space "shm"
//  if pid = owner remove from shmTable, defrag, resize?, free
void shm_destroy(void *addr)
{
  asm volatile("mov r0, %1 \n" // assign r0 =  addr
               "svc %0     \n" // make system call SYS_KILL
               :
               : "I"(SYS_SHM_DESTROY), "r"(addr)
               : "r0");

  return;
}

//  atomic write to shared memory space
void shm_write(void *addr, int data, size_t dataSize)
{
  asm volatile("mov r0, %1 \n" // assign r0 =  addr
               "mov r1, %2 \n" // assign r1 =  addr
               "mov r2, %3 \n" // assign r2 =   pid
               "svc %0     \n" // make system call SYS_KILL
               :
               : "I"(SYS_SHM_WRITE), "r"(addr), "r"(data), "r"(dataSize)
               : "r0", "r1", "r2");

  return;
}
