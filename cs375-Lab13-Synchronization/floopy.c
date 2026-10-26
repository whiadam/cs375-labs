#include <pthread.h>
#include <stdio.h>

typedef struct account_t {
    pthread_mutex_t lock;
    int balance;
    long uuid;
} account_t;

typedef struct {
    account_t *donor;
    account_t *recipient;
    float amount;
} transfer_args_t;

void transfer(account_t *donor, account_t *recipient, float amount) {
    // Lock accounts in order based on uuid to avoid deadlock
    account_t *first = (donor->uuid < recipient->uuid) ? donor : recipient;
    account_t *second = (donor->uuid < recipient->uuid) ? recipient : donor;

    pthread_mutex_lock(&first->lock);
    pthread_mutex_lock(&second->lock);

    if (donor->balance < amount) {
        printf("Insufficient funds.\n");
    } else {
        donor->balance -= amount;
        recipient->balance += amount;
        printf("Transferred %f from account %ld to %ld\n", amount, donor->uuid, recipient->uuid);
    }

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void* transfer_thread(void* arg) {
    transfer_args_t *args = (transfer_args_t*)arg;
    transfer(args->donor, args->recipient, args->amount);
    return NULL;
}

int main() {
    account_t acc1 = {PTHREAD_MUTEX_INITIALIZER, 1000, 1};
    account_t acc2 = {PTHREAD_MUTEX_INITIALIZER, 500, 2};

    transfer_args_t args1 = {&acc1, &acc2, 200};
    transfer_args_t args2 = {&acc2, &acc1, 100};

    pthread_t t1, t2;
    pthread_create(&t1, NULL, transfer_thread, &args1);
    pthread_create(&t2, NULL, transfer_thread, &args2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Final balance of account %ld: %d\n", acc1.uuid, acc1.balance);
    printf("Final balance of account %ld: %d\n", acc2.uuid, acc2.balance);
    return 0;
}
