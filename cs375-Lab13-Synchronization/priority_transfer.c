#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

typedef struct account_t {
    pthread_mutex_t lock;
    int balance;
    long uuid;
    int priority;
} account_t;

typedef struct {
    account_t* donor;
    account_t* recipient;
    float amount;
    int priority;
} transfer_args_t;

pthread_mutex_t prio_lock = PTHREAD_MUTEX_INITIALIZER;

void lock_with_donation(account_t *acc, int thread_priority) {
    if (pthread_mutex_trylock(&acc->lock) != 0) {
        pthread_mutex_lock(&prio_lock);
        if (acc->priority < thread_priority) {
            printf("Priority %d thread is blocked on account %ld, donating priority to the holder (holder had %d)\n",
                   thread_priority, acc->uuid, acc->priority);
            acc->priority = thread_priority;
        }
        pthread_mutex_unlock(&prio_lock);
        pthread_mutex_lock(&acc->lock);
    }
    pthread_mutex_lock(&prio_lock);
    if (acc->priority < thread_priority) acc->priority = thread_priority;
    pthread_mutex_unlock(&prio_lock);
}

int effective_priority(account_t *a, account_t *b, int base) {
    pthread_mutex_lock(&prio_lock);
    int p = base;
    if (a->priority > p) p = a->priority;
    if (b->priority > p) p = b->priority;
    pthread_mutex_unlock(&prio_lock);
    return p;
}

void transfer(account_t *donor, account_t *recipient, float amount, int thread_priority) {
    account_t *first = (donor->uuid < recipient->uuid) ? donor : recipient;
    account_t *second = (donor->uuid < recipient->uuid) ? recipient : donor;

    lock_with_donation(first, thread_priority);
    lock_with_donation(second, thread_priority);

    printf("Priority %d thread holds accounts %ld and %ld\n", thread_priority, first->uuid, second->uuid);
    usleep(200000);
    printf("Priority %d thread is running at effective priority %d\n",
           thread_priority, effective_priority(first, second, thread_priority));

    if (donor->balance < amount) {
        printf("Insufficient funds.\n");
    } else {
        donor->balance -= amount;
        recipient->balance += amount;
        printf("Transferred %f from account %ld to %ld\n", amount, donor->uuid, recipient->uuid);
    }

    pthread_mutex_lock(&prio_lock);
    second->priority = 0; // Reset priority
    first->priority = 0;
    pthread_mutex_unlock(&prio_lock);
    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void* transfer_thread(void* arg) {
    transfer_args_t *params = (transfer_args_t*)arg;
    transfer(params->donor, params->recipient, params->amount, params->priority);
    return NULL;
}

int main() {
    account_t acc1 = {PTHREAD_MUTEX_INITIALIZER, 1000, 1, 0};
    account_t acc2 = {PTHREAD_MUTEX_INITIALIZER, 500, 2, 0};

    transfer_args_t high = {&acc1, &acc2, 200, 2};
    transfer_args_t low = {&acc2, &acc1, 100, 1};

    pthread_t t1, t2;
    pthread_create(&t2, NULL, transfer_thread, &low);
    usleep(50000);
    pthread_create(&t1, NULL, transfer_thread, &high);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Final balance of account %ld: %d\n", acc1.uuid, acc1.balance);
    printf("Final balance of account %ld: %d\n", acc2.uuid, acc2.balance);
    return 0;
}
