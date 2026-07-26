#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int mails = 19;
pthread_mutex_t mutex;

void* add(void *arg)
{
    int i;

    i = *(int*)arg;
    while (i >= 0)
    {
        pthread_mutex_lock(&mutex);
        mails++;
        pthread_mutex_unlock(&mutex);
        i--;
    }
    
    return (NULL);
}

void* decreasse(void *arg)
{
    int i;

    i = *(int *)arg;
    while (i >= 0)
    {
        pthread_mutex_lock(&mutex);
        mails--;
        pthread_mutex_unlock(&mutex);
        i--;
    }
    return (NULL);
}

int main()
{
    pthread_t p1, p2;
    pthread_mutex_init(&mutex, NULL);
    int i = 1000000;

    if (pthread_create(&p1, NULL, add, &i) != 0)
        return (1);
    if (pthread_create(&p2, NULL, &decreasse, &i) != 0)
        return (2);
    if (pthread_join(p1, NULL) != 0)
        return (3);
    if (pthread_join(p2, NULL) != 0)
        return (4);
    pthread_mutex_destroy(&mutex);
    printf("Number of mails: %d\n", mails);
}