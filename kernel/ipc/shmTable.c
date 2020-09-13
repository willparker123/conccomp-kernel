#include "./shmTable.h"
#include <stdlib.h>

shm_t *shmTable;
int shmTabSize = 0;
int shmTabEntries = 0;

/*  makes sure all shmTable entries are adjacent 
    O(n^2)  */
void shmTabDefrag()
{
    for (int i = 0; i < shmTabSize; i++)
    {
        if (shmTable[i].addr == 0)
        {
            for (int j = i + 1; j < shmTabSize; j++)
            {
                if (shmTable[j].addr != 0)
                {
                    memcpy(&shmTable[i], &shmTable[j], sizeof(shm_t)); // set procTable[i] to procTable[j]
                    memset(&shmTable[j], 0, sizeof(shm_t));            // set procTable[j] to 0
                    break;
                }
            }
        }
    }
    return;
}

//  returns index of addr in the shmTable (-1 if not in shmTable)
int shmTabContains(void *addr)
{
    for (int i = 0; i < shmTabSize; i++)
    {
        if (shmTable[i].addr == addr)
        {
            return i;
        }
    }
    return -1;
}

//  initialises shm_t entry in shmTable and initialises shmTable
//    -dynamically resizing
int shmTabInit(pid_t owner, size_t size)
{

    //  if the process table is full, resize
    if (shmTabEntries >= shmTabSize && shmTabSize > 0)
    {
        shmTabSize = shmTabSize * 2;
        shmTable = realloc(shmTable, shmTabSize * sizeof(shm_t));
    }
    else
    {
        shmTabDefrag();
    }
    //  if this is the first entry, malloc
    if (shmTabSize == 0)
    {
        shmTable = malloc(sizeof(shm_t));
        shmTabSize++;
    }

    shmTable[shmTabEntries].addr = calloc(1, size);
    shmTable[shmTabEntries].owner = owner;
    shmTable[shmTabEntries].size = size;

    shmTabEntries++;

    return shmTabEntries - 1;
}

//  delete addr from the shmTable and shrink the shmTable if it has empty space
void shmTabDelete(void *addr)
{
    int index = shmTabContains(addr);
    //  if process is in process table
    if (index >= 0)
    {
        //  clear shmTable entry
        memset(&shmTable[index], 0, sizeof(shm_t));
        shmTabEntries--;
        //  defrag shmTable
        shmTabDefrag();
        if (shmTabEntries <= (shmTabSize / 2))
        {
            //  shrink shmTable if it can be
            shmTabSize = shmTabSize / 2;
            shmTable = realloc(shmTable, shmTabSize * sizeof(shm_t));
        }
    }
    return;
}

//  delete shm with owner = given PID from the shmTable and shrink the shmTable if it has empty space
void shmTabRemove(pid_t pid)
{
    for (int i = 0; i < shmTabSize; i++)
    {
        if (shmTable[i].owner == pid)
        {
            free(shmTable[i].addr);
            memset(&shmTable[i], 0, sizeof(shm_t));
            shmTabEntries--;
            //  defrag shmTable
            shmTabDefrag();
            if (shmTabEntries <= (shmTabSize / 2))
            {
                //  shrink shmTable if it can be
                shmTabSize = shmTabSize / 2;
                shmTable = realloc(shmTable, shmTabSize * sizeof(shm_t));
            }
        }
    }
    return;
}