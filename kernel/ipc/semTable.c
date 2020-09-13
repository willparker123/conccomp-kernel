#include "./semTable.h"
#include <stdlib.h>

semb_t *semTable;
int semTabSize = 0;
int semTabEntries = 0;

/*  makes sure all semTable entries are adjacent 
    O(n^2)  */
void semTabDefrag()
{
    for (int i = 0; i < semTabSize; i++)
    {
        if (semTable[i].waitingPid == 0)
        {
            for (int j = i + 1; j < semTabSize; j++)
            {
                if (semTable[i].waitingPid != 0)
                {
                    memcpy(&semTable[i], &semTable[j], sizeof(semb_t)); // set procTable[i] to procTable[j]
                    memset(&semTable[j], 0, sizeof(semb_t));            // set procTable[j] to 0
                    break;
                }
            }
        }
    }
    return;
}

//  returns index of addr in the semTable (-1 if not in semTable)
int semTabContains(sem_t sem)
{
    for (int i = 0; i < semTabSize; i++)
    {
        if (semTable[i].sem == sem && semTable[i].waitingPid != 0)
        {
            return i;
        }
    }
    return -1;
}

//  gets the PID of the owner of the semaphore
pid_t semGetOwner(sem_t sem)
{
    for (int i = 0; i < semTabSize; i++)
    {
        if (semTable[i].sem == sem)
        {
            return semTable[i].owner;
        }
    }
    return 0;
}

//  returns true if the given (sem, pid, owner) entry is already in the table
bool semTabDuplicate(sem_t sem, pid_t pid, pid_t owner)
{
    for (int i = 0; i < semTabSize; i++)
    {
        if (semTable[i].sem == sem && semTable[i].waitingPid == pid && semTable[i].owner == owner)
        {
            return true;
        }
    }
    return false;
}

//  initialises sem_t entry in semTable and initialises semTable
//    -dynamically resizing
void semTableAdd(sem_t sem, pid_t pid, pid_t owner)
{
    if (!semTabDuplicate(sem, pid, owner))
    {
        //  if the process table is full, resize
        if (semTabEntries >= semTabSize && semTabSize > 0)
        {
            semTabSize = semTabSize * 2;
            semTable = realloc(semTable, semTabSize * sizeof(semb_t));
        }
        else
        {
            semTabDefrag();
        }
        //  if this is the first entry, malloc
        if (semTabSize == 0)
        {
            semTable = malloc(sizeof(semb_t));
            semTabSize++;
        }

        semTable[semTabEntries].sem = sem;
        semTable[semTabEntries].waitingPid = pid;
        semTable[semTabEntries].owner = owner;

        semTabEntries++;
    }
    return;
}

//  set status of all waiting processes to STATUS_READY and removes entry from semTable
void semTableNotify(sem_t sem)
{
    int index = semTabContains(sem);

    while (index >= 0)
    {
        if (semTable[index].waitingPid != 0)
        {
            //  clear semTable entry
            procTable[procTableContains(semTable[index].waitingPid)].status = STATUS_READY;
            memset(&semTable[index], 0, sizeof(semb_t));
            semTabEntries--;
            //  defrag semTable
            semTabDefrag();
            if (semTabEntries <= (semTabSize / 2))
            {
                //  shrink semTable if it can be
                semTabSize = semTabSize / 2;
                semTable = realloc(semTable, semTabSize * sizeof(semb_t));
            }

            index = semTabContains(sem);
        }
    }
    return;
}

//  deletes all entries with waitingPid = given PID (completely removes semaphore if owner = given PID)
void semTableRemove(pid_t pid)
{
    for (int i = 0; i < semTabSize; i++)
    {
        if (semTable[i].waitingPid == pid)
        {
            //  clear semTable entry
            memset(&semTable[i], 0, sizeof(semb_t));
            semTabEntries--;
            //  defrag semTable
            semTabDefrag();
            if (semTabEntries <= (semTabSize / 2))
            {
                //  shrink semTable if it can be
                semTabSize = semTabSize / 2;
                semTable = realloc(semTable, semTabSize * sizeof(semb_t));
            }
        }
        else if (semTable[i].waitingPid == 0 && semTable[i].owner == pid)
        {
            //  clear semTable entry
            memset(&semTable[i], 0, sizeof(semb_t));
            semTabEntries--;
            //  defrag semTable
            semTabDefrag();
            if (semTabEntries <= (semTabSize / 2))
            {
                //  shrink semTable if it can be
                semTabSize = semTabSize / 2;
                semTable = realloc(semTable, semTabSize * sizeof(semb_t));
            }
            free(semTable[i].sem);
        }
    }

    return;
}