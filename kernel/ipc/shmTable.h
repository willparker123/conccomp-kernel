#ifndef __SHMTABLE_H
#define __SHMTABLE_H

#include "../hilevel/hilevel.h"
#include "../../user/libc.h"
#include "../processTables/processTable.h"

extern shm_t *shmTable;
extern int shmTabSize;
extern int shmTabEntries;

extern int shmTabInit(pid_t owner, size_t size);
extern void shmTabDelete(void *addr);
extern int shmTabContains(void *addr);
extern void shmTabRemove(pid_t pid);

#endif
