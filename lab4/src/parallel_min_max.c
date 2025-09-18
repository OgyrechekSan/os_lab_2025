#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1; // Новый параметр - таймаут
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0}, // Новый параметр
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3: // Обработка timeout
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be a positive number\n");
              return 1;
            }
            break;
          case 4:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Массивы для хранения PID дочерних процессов
  pid_t *child_pids = malloc(pnum * sizeof(pid_t));

  // Create arrays for pipes or file descriptors
  int *pipe_fds = NULL;
  char **filenames = NULL;
  
  if (!with_files) {
    pipe_fds = malloc(2 * pnum * sizeof(int));
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipe_fds + 2*i) == -1) {
        printf("Pipe creation failed!\n");
        return 1;
      }
    }
  } else {
    filenames = malloc(pnum * sizeof(char*));
    for (int i = 0; i < pnum; i++) {
      filenames[i] = malloc(25 * sizeof(char));
      snprintf(filenames[i], 25, "min_max_%d.txt", i);
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создаем дочерние процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[i] = child_pid; // Сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // child process
        unsigned int start_index = i * array_size / pnum;
        unsigned int end_index = (i + 1) * array_size / pnum;
        
        if (end_index > array_size) {
          end_index = array_size;
        }

        struct MinMax local_min_max = GetMinMax(array, start_index, end_index);

        if (with_files) {
          FILE *file = fopen(filenames[i], "w");
          if (file == NULL) {
            printf("Failed to open file %s\n", filenames[i]);
            exit(1);
          }
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          close(pipe_fds[2*i]);
          write(pipe_fds[2*i + 1], &local_min_max.min, sizeof(int));
          write(pipe_fds[2*i + 1], &local_min_max.max, sizeof(int));
          close(pipe_fds[2*i + 1]);
        }
        free(array);
        exit(0);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Parent process
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipe_fds[2*i + 1]);
    }
  }

  // Ожидание с таймаутом
  if (timeout > 0) {
    printf("Waiting with timeout %d seconds...\n", timeout);
    
    // Ждем завершения процессов с таймаутом
    int time_elapsed = 0;
    while (active_child_processes > 0 && time_elapsed < timeout) {
      sleep(1);
      time_elapsed++;
      
      // Проверяем, завершились ли какие-то процессы
      pid_t finished_pid;
      int status;
      while ((finished_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        active_child_processes -= 1;
        if (WIFEXITED(status)) {
          printf("Child process %d finished normally\n", finished_pid);
        }
      }
    }

    // Если таймаут истек, убиваем оставшиеся процессы
    if (active_child_processes > 0) {
      printf("Timeout reached! Sending SIGKILL to %d remaining child processes\n", active_child_processes);
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
          // Проверяем, существует ли процесс еще
          if (kill(child_pids[i], 0) == 0) {
            printf("Killing child process %d\n", child_pids[i]);
            kill(child_pids[i], SIGKILL);
          }
        }
      }
      
      // Ждем завершения убитых процессов
      while (active_child_processes > 0) {
        wait(NULL);
        active_child_processes -= 1;
      }
    }
  } else {
    // Обычное ожидание без таймаута
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      FILE *file = fopen(filenames[i], "r");
      if (file != NULL) {
        fscanf(file, "%d %d", &min, &max);
        fclose(file);
        remove(filenames[i]);
      }
    } else {
      read(pipe_fds[2*i], &min, sizeof(int));
      read(pipe_fds[2*i], &max, sizeof(int));
      close(pipe_fds[2*i]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  // Cleanup
  free(array);
  free(child_pids);
  if (pipe_fds != NULL) free(pipe_fds);
  if (filenames != NULL) {
    for (int i = 0; i < pnum; i++) {
      free(filenames[i]);
    }
    free(filenames);
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}