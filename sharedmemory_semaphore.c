#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
 
void wait_sem(int semid)
{
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}
 
void signal_sem(int semid)
{
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}
 
int main()
{
    key_t key = 555;
 
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    char *data = (char *)shmat(shmid, NULL, 0);
 
    int semid = semget(key, 1, 0666 | IPC_CREAT);
    semctl(semid, 0, SETVAL, 1);
 
    pid_t pid = fork();
 
    if (pid == 0)
    {
        sleep(1);
        wait_sem(semid);
        printf("Reader read: %s\n", data);
        signal_sem(semid);
        shmdt(data);
    }
    else
    {
        wait_sem(semid);
        strcpy(data, "Shared memory protected by semaphore");
        printf("Writer written data\n");
        signal_sem(semid);
 
        wait(NULL);
 
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
    }
 
    return 0;
}
