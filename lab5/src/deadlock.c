#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_function(void* arg) {
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");
    
    sleep(1);// Даем время второму потоку захватить mutex2
    
    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);  // ← DEADLOCK: mutex2 уже захвачен thread2
    printf("Thread 1: Locked mutex2\n");
    
    // Критическая секция
    printf("Thread 1: Working with both mutexes\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для второго потока  
void* thread2_function(void* arg) {
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");
    
    sleep(1);// Даем время первому потоку захватить mutex1
    
    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);  // ← DEADLOCK: mutex1 уже захвачен thread1
    printf("Thread 2: Locked mutex1\n");
    
    // Критическая секция
    printf("Thread 2: Working with both mutexes\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("=== Deadlock Demonstration ===\n");
    printf("Сейчас будет показана классическая реализация deadlock.\n\n");
    
    // Создаем два потока
    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);
    
    // Даем потокам время поработать,чтобы увидеть deadlock
    sleep(3);
    
    printf("\nMain: Кажется был пойман deadlock...\n");
    printf("Ctrl+C, чтобы выйти.\n");
    
    // Ждем завершения потоков (которого никогда не произойдет)
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Это никто не увидит!\n");
    
    return 0;
}