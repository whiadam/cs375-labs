#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_CHAIRS 3
#define NUM_CUSTOMERS 5

sem_t customers, barber, chairs;
pthread_mutex_t mutex;

void *barber_thread(void *arg) {
    while (1) {
        sem_wait(&customers);
        pthread_mutex_lock(&mutex);
        sem_post(&chairs);
        pthread_mutex_unlock(&mutex);
        printf("Barber is cutting hair.\n");
        sleep(2);
        sem_post(&barber);
    }
    return NULL;
}

void *customer_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    if (sem_trywait(&chairs) == 0) {
        sem_post(&customers);
        pthread_mutex_unlock(&mutex);
        sem_wait(&barber);
        printf("Customer got haircut.\n");
    } else {
        pthread_mutex_unlock(&mutex);
        printf("Customer left (no chairs).\n");
    }
    return NULL;
}

int main() {
    pthread_t barber_t, customers_t[NUM_CUSTOMERS];
    sem_init(&customers, 0, 0);
    sem_init(&barber, 0, 0);
    sem_init(&chairs, 0, NUM_CHAIRS);
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&barber_t, NULL, barber_thread, NULL);

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_create(&customers_t[i], NULL, customer_thread, NULL);
        sleep(1);
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers_t[i], NULL);
    }

    // Note: Barber thread runs indefinitely; in practice, add termination logic.
    sem_destroy(&customers);
    sem_destroy(&barber);
    sem_destroy(&chairs);
    pthread_mutex_destroy(&mutex);
    return 0;
}
