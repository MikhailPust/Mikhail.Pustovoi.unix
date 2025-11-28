#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
 
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;
 
bool ready = false;   // признак наступления события
int  N     = 10;      // сколько событий генерируем
 
void* producer(void *arg)
{
    (void)arg;  // аргумент не используем
 
    for (int i = 0; i < N; ++i) {
        sleep(1);   // имитация события раз в секунду
 
        pthread_mutex_lock(&mtx);
 
        // если предыдущее событие не обработано – ждём следующую попытку
        if (ready) {
            pthread_mutex_unlock(&mtx);
            i--;            // не считаем это событие, попробуем позже
            continue;
        }
 
        ready = true;       // событие наступило
        printf("[producer] event %d\n", i + 1);
 
        pthread_cond_signal(&cv);   // будим потребителя
        pthread_mutex_unlock(&mtx);
    }
 
    return NULL;
}
 
void* consumer(void *arg)
{
    (void)arg;
 
    for (int i = 0; i < N; ++i) {
        pthread_mutex_lock(&mtx);
 
        // ждём, пока появится новое событие
        while (!ready) {
            pthread_cond_wait(&cv, &mtx);
        }
 
        // событие «обработано»
        printf("[consumer] got event %d\n", i + 1);
        ready = false;
 
        pthread_mutex_unlock(&mtx);
    }
 
    return NULL;
}
 
int main(void)
{
    pthread_t prod, cons;
 
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
 
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
 
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cv);
 
    return 0;
}
