#include "diningPhil.h"

int philosophers = 16;

int *addrSpace[8];
sem_t sems[16];

void philOp(int philId)
{
    sem_t leftSem;
    sem_t rightSem;
    if (philId != 0)
    {
        if (philId < philosophers)
        {
            leftSem = sems[philId - 1];
            rightSem = sems[philId];
        }
        else
        {
            leftSem = sems[philId - 1];
            rightSem = sems[0];
        }
    }

    while (1)
    {
        if (philId != 0)
        {
            //  if odd ID
            if (philId % 2 == 1)
            {
                /*
                int value;
                value = *(int *)addrSpace[philId / 2];
                value++;
                shm_write(addrSpace[philId / 2], value, sizeof(int));*/
                sem_post(rightSem);
                sem_wait(leftSem);
            }
            else // if even ID
            {
                sem_wait(leftSem);
                /*
                int value;
                value = *(int *)addrSpace[(philId / 2) - 1];
                value++;
                shm_write(addrSpace[(philId / 2) - 1], value, sizeof(int));
                */
                sem_post(rightSem);
            }
        }
    }
    return;
}

void main_diningPhil()
{
    int philId = 0; //  0 = PARENT
    //  if this is the parent process, allocate a shared integer array of length 8
    //  if this is the parent process, allocate an array of semaphores
    for (int i = 0; i < philosophers / 2; i++)
    {
        /*
        void *addrSpaceItem = shm_init(sizeof(int));
        addrSpace[i] = addrSpaceItem;
        */
        sems[i] = sem_init();
        sems[i + (philosophers / 2)] = sem_init();
    }

    //  fork process 16 times and set philId = 1...16 and give each philosopher a left and right neighbour (semaphore)
    for (int j = 1; j <= philosophers; j++)
    {
        philId = j;

        if (fork() == 0)
        {
            break;
        }
    }
    philOp(philId);
    //  do philOp indefinitely (increment memSpace[i] or wait)

    exit(EXIT_SUCCESS);
}
