#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pthread.h>
#include "utils.h"
#include "sum.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  long long *result = malloc(sizeof(long long)); 
  *result = Sum(sum_args);
  return (void*)result;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--threads_num") == 0 && i + 1 < argc) {
      threads_num = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--array_size") == 0 && i + 1 < argc) {
      array_size = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
      seed = atoi(argv[++i]);
    }
  }
  
  if (threads_num == 0 || array_size == 0) {
    printf("Usage: %s --threads_num <num> --array_size <num> --seed <num>\n", argv[0]);
    return 1;
  }
  
  // Generate array
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];
  
  // Calculate chunk size for each thread
  int chunk_size = array_size / threads_num;
  
  // Prepare arguments for each thread
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
  }
  
  // Start timing
  clock_t start_time = clock();
  
  // Create threads
  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }
  
  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    long long *sum_ptr;
    pthread_join(threads[i], (void **)&sum_ptr);
    total_sum += *sum_ptr;
    free(sum_ptr); 
  }
  
  // End timing
  clock_t end_time = clock();
  double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  
  free(array);
  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %.6f seconds\n", elapsed_time);
  
  return 0;
}