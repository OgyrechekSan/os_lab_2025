#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main() {
    printf("Демонстрация зомби-процессов\n");
    
    printf("Создаем зомби-процесс на 10 секунд...\n");
    

    //создаём дочерний процесс
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс PID: %d\n", getpid());
        printf("Дочерний процесс завершается через 3 секунды...\n");
        sleep(3);
        printf("Дочерний процесс завершен (станет зомби)\n");
        exit(0);
    } else {
        // Родительский процесс не вызывает wait() - создаем зомби
        printf("Родительский процесс PID: %d\n", getpid());
        printf("Дочерний процесс создан с PID: %d\n", pid);
        printf("Родитель не вызывает wait() - ждем 10 секунд...\n");
        sleep(10);
        
    }
    
    return 0;
}