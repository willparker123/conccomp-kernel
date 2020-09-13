#ifndef __SEMTABLE_H
#define __SEMTABLE_H

#include "../hilevel/hilevel.h"
#include "../../user/libc.h"
#include "../processTables/processTable.h"

typedef struct
{
    sem_t sem;
    pid_t waitingPid;
    int owner;
} semb_t;

extern semb_t *semTable;
extern int semTabSize;
extern int semTabEntries;

extern void semTableNotify(sem_t sem);
extern void semTableAdd(sem_t sem, pid_t pid, pid_t owner);
extern int semTabContains(sem_t sem);
extern bool semTabDuplicate(sem_t sem, pid_t pid, pid_t owner);
extern pid_t semGetOwner(sem_t sem);
extern void semTableRemove(pid_t pid);

#endif
