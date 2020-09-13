#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "../hilevel/hilevel.h"
#include "../processTables/processTable.h"
#include "../processTables/processTableHistory.h"

// maximum levels in the Multi-level Feedback Queue (excludes Round Robin final level)
extern int MAX_QUEUE_LEVELS;

typedef struct
{
    pid_t *processes;
    int length;
    int size;
    int id;
    int t;
} queue_t;

typedef struct
{
    queue_t *queueRoundRobin;
    queue_t *queuesFCFS;
    int levels;
    int processCount;
    bool enabled;
} mlfq_t;

typedef struct
{
    int index;
    int level;
} qpos_t;

extern void dispatch(ctx_t *ctx, pcb_t *prev, pcb_t *next);
extern void schedule(ctx_t *ctx);
extern void invokeQueueMLFQ();
extern void removeFromMLFQ(pid_t pid);
extern void deleteMLFQ();
extern void enableMLFQ();
extern void disableMLFQ();
extern void addToMLFQ(pid_t pid);

#endif
