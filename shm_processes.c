#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct {
    int BankAccount;
    sem_t mutex;
} shared_data;

int main()
{
    srand(time(NULL) ^ getpid());

    /* ---- CREATE SHARED MEMORY ---- */
    shared_data *shm = mmap(NULL, sizeof(shared_data),
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS,
                            -1, 0);

    if(shm == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    /* Initialize shared memory variables */
    shm->BankAccount = 0;

    /* ---- INITIALIZE SEMAPHORE ---- */
    if(sem_init(&shm->mutex, 1, 1) < 0){
        perror("sem_init");
        exit(1);
    }

    pid_t pid = fork();

    if(pid < 0){
        perror("fork");
        exit(1);
    }

    /* ================================================================= */
    /* ======================= PARENT PROCESS ========================== */
    /* ================================================================= */
    if(pid > 0)
    {
        while(1)
        {
            sleep(rand() % 6);       // 0–5 seconds
            printf("Dear Old Dad: Attempting to Check Balance\n");

            sem_wait(&shm->mutex);
            int localBalance = shm->BankAccount;
            sem_post(&shm->mutex);

            int r = rand() % 2;      // even or odd

            if(r == 0)   /* even → possible deposit */
            {
                if(localBalance < 100)
                {
                    // Attempt deposit
                    sem_wait(&shm->mutex);
                    localBalance = shm->BankAccount;     // re-read in critical section

                    int amount = rand() % 101;           // 0–100
                    if(amount % 2 == 0)                  // deposit only if even
                    {
                        localBalance += amount;
                        printf("Dear Old Dad: Deposits $%d / Balance = $%d\n",
                               amount, localBalance);
                        shm->BankAccount = localBalance;
                    }
                    else
                    {
                        printf("Dear Old Dad: Doesn't have any money to give\n");
                    }

                    sem_post(&shm->mutex);
                }
                else
                {
                    printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n",
                           localBalance);
                }
            }
            else  /* odd → just check */
            {
                printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
            }
        }
    }

    /* ================================================================= */
    /* ======================= CHILD PROCESS =========================== */
    /* ================================================================= */
    else
    {
        while(1)
        {
            sleep(rand() % 6);    // 0–5 seconds
            printf("Poor Student: Attempting to Check Balance\n");

            sem_wait(&shm->mutex);
            int localBalance = shm->BankAccount;
            sem_post(&shm->mutex);

            int r = rand() % 2;

            if(r == 0)  /* even → attempt withdrawal */
            {
                sem_wait(&shm->mutex);
                localBalance = shm->BankAccount;   // re-read

                int need = rand() % 51;            // 0–50
                printf("Poor Student needs $%d\n", need);

                if(need <= localBalance)
                {
                    localBalance -= need;
                    printf("Poor Student: Withdraws $%d / Balance = $%d\n",
                           need, localBalance);
                    shm->BankAccount = localBalance;
                }
                else
                {
                    printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
                }

                sem_post(&shm->mutex);
            }
            else  /* odd → check only */
            {
                printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
            }
        }
    }

    return 0;
}

