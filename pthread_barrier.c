#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#define THREADS 5
pthread_barrier_t barrier;
void* thread_func(void* arg) {
    int id = *(int*)arg;
    printf("Thread %d: doing some work\n", id);
    sleep(id);   
    printf("Thread %d: waiting at barrier\n", id);
    pthread_barrier_wait(&barrier);
    printf("Thread %d: crossed the barrier\n", id);
 
    return NULL;
}
 
int main() {
    pthread_t threads[THREADS];
    int ids[THREADS];
 
    pthread_barrier_init(&barrier, NULL, THREADS);
 
    for (int i = 0; i < THREADS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_func, &ids[i]);
    }
 
    for (int i = 0; i < THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
    }
    pthread_barrier_destroy(&barrier);
    return 0;
}
