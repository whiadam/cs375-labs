#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

int numBatterInBowl = 0;
int numEggInBowl = 0;
bool readyToEat = false;
int cakesBaked = 0;
pthread_mutex_t lock;
pthread_cond_t needIngredients, readyToBake, startEating;

void addBatter() {
    numBatterInBowl += 1;
    printf("Added batter (batter: %d, eggs: %d)\n", numBatterInBowl, numEggInBowl);
}

void addEgg() {
    numEggInBowl += 1;
    printf("Added egg (batter: %d, eggs: %d)\n", numBatterInBowl, numEggInBowl);
}

void heatBowl() {
    readyToEat = true;
    numBatterInBowl = 0;
    numEggInBowl = 0;
    cakesBaked += 1;
    printf("Baking cake #%d\n", cakesBaked);
}

void eatCake() {
    readyToEat = false;
    printf("Ate cake #%d\n", cakesBaked);
}

void* batterAdder(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (numBatterInBowl || readyToEat) {
            pthread_cond_wait(&needIngredients, &lock);
        }
        addBatter();
        pthread_cond_signal(&readyToBake);
        pthread_mutex_unlock(&lock);
        pthread_mutex_lock(&lock);
    }
}

void* eggBreaker(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (numEggInBowl >= 2 || readyToEat) {
            pthread_cond_wait(&needIngredients, &lock);
        }
        addEgg();
        pthread_cond_signal(&readyToBake);
        pthread_mutex_unlock(&lock);
        pthread_mutex_lock(&lock);
    }
}

void* bowlHeater(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (numBatterInBowl < 1 || numEggInBowl < 2 || readyToEat) {
            pthread_cond_wait(&readyToBake, &lock);
        }
        heatBowl();
        pthread_cond_signal(&startEating);
        pthread_mutex_unlock(&lock);
        pthread_mutex_lock(&lock);
    }
}

void* cakeEater(void* arg) {
    pthread_mutex_lock(&lock);
    while (1) {
        while (!readyToEat) {
            pthread_cond_wait(&startEating, &lock);
        }
        eatCake();
        pthread_cond_broadcast(&needIngredients);
        pthread_mutex_unlock(&lock);
        pthread_mutex_lock(&lock);
    }
}

int main(int argc, char *argv[]) {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&needIngredients, NULL);
    pthread_cond_init(&readyToBake, NULL);
    pthread_cond_init(&startEating, NULL);

    pthread_t batter, egg1, egg2, heater, eater;
    pthread_create(&batter, NULL, batterAdder, NULL);
    pthread_create(&egg1, NULL, eggBreaker, NULL);
    pthread_create(&egg2, NULL, eggBreaker, NULL);
    pthread_create(&heater, NULL, bowlHeater, NULL);
    pthread_create(&eater, NULL, cakeEater, NULL);

    while (1) sleep(1); // Sleep forever

    return 0;
}
