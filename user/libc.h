/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __LIBC_H
#define __LIBC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// Define a type that that captures a Process IDentifier (PID).

typedef int pid_t;

/* The definitions below capture symbolic constants within these classes:
 *
 * 1. system call identifiers (i.e., the constant used by a system call
 *    to specify which action the kernel should take),
 * 2. signal identifiers (as used by the kill system call), 
 * 3. status codes for exit,
 * 4. standard file descriptors (e.g., for read and write system calls),
 * 5. platform-specific constants, which may need calibration (wrt. the
 *    underlying hardware QEMU is executed on).
 *
 * They don't *precisely* match the standard C library, but are intended
 * to act as a limited model of similar concepts.
 */

#define SYS_YIELD (0x00)
#define SYS_WRITE (0x01)
#define SYS_READ (0x02)
#define SYS_FORK (0x03) //   TODO
#define SYS_EXIT (0x04)
#define SYS_EXEC (0x05)
#define SYS_KILL (0x06)
#define SYS_PRIO (0x07) //   TODO increase priority in scheduler
#define SYS_PAUSE (0x08)
#define SYS_UNPAUSE (0x09)
#define SYS_STATUS (0x10)
#define SYS_HISTORY (0x11)
#define SYS_CLOSE (0x12)
#define SYS_FORKPROC (0x13)
#define SYS_GETADDR (0x14)

#define EXIT_SUCCESS 0 //EXIT W SUCCESS
#define EXIT_FAILURE 1 //EXIT W FAILURE (LOG PCB ENTRY IN procTableHistory)

#define STDIN_FILENO (0)
#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

#define SYS_SHM_INIT (0x20)
#define SYS_SHM_DESTROY (0x21)
#define SYS_SHM_WRITE (0x22)

#define SYS_SEM_INIT (0x30)
#define SYS_SEM_DESTROY (0x31)
#define SYS_SEM_POST (0x32)
#define SYS_SEM_WAIT (0x33)

typedef int *sem_t;

typedef struct
{
    int owner; // PID of owner
    size_t size;
    void *addr; // address of shared mem space
} shm_t;

// convert ASCII string x into integer r
extern int atoiLocal(char *x);
// convert integer x into ASCII string r
extern void itoaLocal(char *r, int x);

// cooperatively yield control of processor, i.e., invoke the scheduler
extern void yield(pid_t pid);

// write n bytes from x to   the file descriptor fd; return bytes written
extern int write(int fd, const void *x, size_t n);
// read  n bytes into x from the file descriptor fd; return bytes read
extern int read(int fd, void *x, size_t n);

// perform fork, returning 0 iff. child or > 0 iff. parent process
extern int fork();

extern int forkProc(pid_t pid);
// perform exit, i.e., terminate process with status x
extern void exit(int x);
// perform exec, i.e., start executing program at address x
extern void exec(const void *x);

// for process identified by pid, send signal of x
extern int kill(pid_t pid, int x);
// for process identified by pid, set  priority to x
extern void nice(pid_t pid, int p);

// for process identified by pid, pause execution and scheduling but remains in processTable
extern void pause(pid_t pid);

// for process identified by pid, resumes execution and scheduling but remains in processTable
extern void unpause(pid_t pid);

// shows the status of the process table (entries)
extern void status();

// shows the status of the process table history (entries)
extern void history();
// close system
extern void close(int x);

// return gpr[0]
extern int getaddr();

extern void *shm_init(size_t size);
extern void shm_destroy(void *addr);
extern void shm_write(void *addr, int data, size_t dataSize);
extern sem_t sem_init();
extern void sem_destroy(sem_t sem);
extern void sem_post(sem_t sem);
extern int sem_wait(sem_t sem);

#endif
