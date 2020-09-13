#ifndef __PROCESSTABLE_H
#define __PROCESSTABLE_H

#include "../hilevel/hilevel.h"

extern pcb_t *procTable;
extern pcb_t *currentProc;
extern int procTabSize;

extern int procInit(void *mainFunc);
extern void procDelete(pid_t pid);
extern int procTableContains(pid_t pid);
extern void dispatch(ctx_t *ctx, pcb_t *prev, pcb_t *next);
extern void schedule(ctx_t *ctx);
int procCopy(pcb_t *parentProc, ctx_t *ctx);
extern int procExec(void *mainFunc);

extern int MAX_PROCS;
extern int PROCS;
extern int PROCS_ACTIVE;

#endif
