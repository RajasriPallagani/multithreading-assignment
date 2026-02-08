#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
 
pthread_mutex_t mutex;
pthread_cond_t cond;
 
int data = 0;      
int flag = 0;   

void* producer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        sleep(1);
 
        pthread_mutex_lock(&mutex);
 
        data = i;
        flag = 1;
 
        printf("Producer produced: %d\n", data);
 
        pthread_cond_signal(&cond);   
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
 

void* consumer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        pthread_mutex_lock(&mutex);
 
        while (flag == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
 
        printf("Consumer consumed: %d\n", data);
        flag = 0;
 
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
 
int main() 
{
    pthread_t prod, cons;
 
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
 
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
 
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
 
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
 
    return 0;
}
