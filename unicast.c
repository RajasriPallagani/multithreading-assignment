#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int allowed_thread = 1;

void* thread_function(void* arg)
{
    int thread_id = *(int*)arg;
    pthread_mutex_lock(&mutex);
    while (thread_id != allowed_thread)
    {
        printf("Thread %d waiting...\n", thread_id);
        pthread_cond_wait(&cond, &mutex);
    }
    printf("Thread %d received\n", thread_id);
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
    allowed_thread = 2;
    printf("Main thread notifying Thread 2\n");
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < 3; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
