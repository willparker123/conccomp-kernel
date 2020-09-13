#ifndef __PROCESSTABLEHISTORY_H
#define __PROCESSTABLEHISTORY_H

#include "../hilevel/hilevel.h"

extern pcb_t *lastExitProc;
extern pcb_t *procTableHistory;
extern int procTabHistorySize;

extern int procHistoryInit(pcb_t *entry);
extern void procHistoryDelete(int entryNumber);

extern int MAX_HISTORY_PROCS;
extern int HISTORY_PROCS;

#endif
