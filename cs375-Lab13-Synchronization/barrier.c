#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 4

pthread_mutex_t lock;
pthread_cond_t cv;
int count = 0;
int generation = 0;

void barrier() {
    pthread_mutex_lock(&lock);
    int my_generation = generation;
    count++;
    if (count < NUM_THREADS) {
        while (my_generation == generation) {
            pthread_cond_wait(&cv, &lock);
        }
    } else {
        count = 0;
        generation++;
        pthread_cond_broadcast(&cv);
    }
    pthread_mutex_unlock(&lock);
}

void* worker(void* arg) {
    int id = *(int*)arg;
    usleep((id + 1) * 200000);
    printf("Thread %d: Before barrier\n", id);
    barrier();
    printf("Thread %d: After barrier\n", id);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cv);
    return 0;
}
