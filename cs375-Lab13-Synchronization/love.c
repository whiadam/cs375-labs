#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock;
pthread_cond_t cv;
int subaru = 0;

void* helper(void* arg) {
    pthread_mutex_lock(&lock);
    subaru += 1;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock);
    pthread_exit(0);
}

int main() {
    pthread_t thread;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    pthread_create(&thread, NULL, helper, NULL);
    pthread_mutex_lock(&lock);
    while (subaru != 1) {
        pthread_cond_wait(&cv, &lock);
    }
    if (subaru == 1)
        printf("I love Emilia!\n");
    else
        printf("I love Rem!\n");
    pthread_mutex_unlock(&lock);

    pthread_join(thread, NULL);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cv);
    return 0;
}
