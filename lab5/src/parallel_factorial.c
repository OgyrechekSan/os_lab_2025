#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// для передачи данных в поток
typedef struct {
    long long start;
    long long end;
    long long mod;
    long long result;
} ThreadData;


long long global_result = 1;
pthread_mutex_t mutex;

// Функция для вычисления частичного факториала
void* calculate_partial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    long long partial = 1;
    
    // Вычисляем произведение чисел в своем диапазоне
    for (long long i = data->start; i <= data->end; i++) {
        partial = (partial * i) % data->mod;
    }
    
    data->result = partial;
    
    // Синхронизированно умножаем на глобальный результат
    pthread_mutex_lock(&mutex);
    global_result = (global_result * partial) % data->mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(int argc, char** argv) {
    long long k = 0;
    int pnum = 0;
    long long mod = 0;
    
    // Парсим аргументы 
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoll(argv[++i]);
        } else if (strcmp(argv[i], "--pnum") == 0 && i + 1 < argc) {
            pnum = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--mod") == 0 && i + 1 < argc) {
            mod = atoll(argv[++i]);
        }
    }
    
    // Проверяем аргументы
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Usage: %s -k NUMBER --pnum NUMBER --mod NUMBER\n", argv[0]);
        printf("Example: %s -k 10 --pnum 4 --mod 1000\n", argv[0]);
        return 1;
    }
    
    // Если потоков больше чем чисел - ограничиваем
    if (pnum > k) {
        pnum = k;
        printf("Adjusted pnum to %d (cannot have more threads than numbers)\n", pnum);
    }
    
    printf("Calculating %lld! mod %lld using %d threads\n\n", k, mod, pnum);
    
    // Инициализируем мьютекс
    pthread_mutex_init(&mutex, NULL);
    
    // Создаем потоки и данные для них
    pthread_t threads[pnum];
    ThreadData thread_data[pnum];
    
    // Разбиваем диапазон на части
    long long numbers_per_thread = k / pnum;
    long long current = 1;
    
    for (int i = 0; i < pnum; i++) {
        thread_data[i].start = current;
        thread_data[i].end = current + numbers_per_thread - 1;
        
        // Последний поток получает все оставшиеся числа
        if (i == pnum - 1) {
            thread_data[i].end = k;
        }
        
        thread_data[i].mod = mod;
        thread_data[i].result = 1;
        
        printf("Thread %d: numbers from %lld to %lld\n", 
               i, thread_data[i].start, thread_data[i].end);
        
        // Создаем поток
        if (pthread_create(&threads[i], NULL, calculate_partial, &thread_data[i]) != 0) {
            printf("Error creating thread %d\n", i);
            return 1;
        }
        
        current = thread_data[i].end + 1;
    }
    
    // Ждем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex);
    
    printf("\nPartial results:\n");
    for (int i = 0; i < pnum; i++) {
        printf("  Thread %d: %lld\n", i, thread_data[i].result);
    }
    
    printf("\nFinal result: %lld! mod %lld = %lld\n", k, mod, global_result);
    
    return 0;
}