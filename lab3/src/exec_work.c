#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        return 1;
    }

    printf("Parent process PID: %d\n", getpid());
    printf("Creating child process...\n");

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Child process
        printf("Child process PID: %d (parent PID: %d)\n", getpid(), getppid());
        printf("Executing: ./sequential_min_max %s %s\n", argv[1], argv[2]);
        
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        execvp("./sequential_min_max", args);
        
        perror("execvp failed");
        exit(1);
    } else {
        // Parent process
        printf("Parent waiting for child PID: %d\n", pid);
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child process %d exited with status: %d\n", pid, WEXITSTATUS(status));
        } else {
            printf("Child process %d terminated abnormally\n", pid);
        }
    }

    printf("Parent process %d completed\n", getpid());
    return 0;
}