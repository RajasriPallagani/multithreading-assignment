#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
 
pthread_mutex_t mutex;
pthread_cond_t cond;
 
int start = 0;
 

void* thread_function(void* arg)
{
    int thread_id = *(int*)arg;
 
    pthread_mutex_lock(&mutex);
 
    while (start == 0)
    {
        printf("Thread %d waiting...\n", thread_id);
        pthread_cond_wait(&cond, &mutex);
    }
 
    printf("Thread %d received broadcast signal\n", thread_id);
 
    pthread_mutex_unlock(&mutex);
    return NULL;
}
 
int main()
{
    pthread_t threads[3];
    int ids[3] = {1, 2, 3};
 
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
 
    for (int i = 0; i < 3; i++)
    {
        pthread_create(&threads[i], NULL, thread_function, &ids[i]);
    }
 
    sleep(2);   
 
    pthread_mutex_lock(&mutex);
    start = 1;
    printf("Main thread broadcasting signal to all threads\n");
    pthread_cond_broadcast(&cond);   
    pthread_mutex_unlock(&mutex);
 
    for (int i = 0; i < 3; i++)
    {
        pthread_join(threads[i], NULL);
    }
 
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
 
    return 0;
}
