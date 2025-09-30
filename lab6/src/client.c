#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "same.h"

struct Server {
  char ip[255];
  int port;
};

struct ServerThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
};



void* ConnectToServer(void* args) {
    struct ServerThreadData* data = (struct ServerThreadData*)args;
    
    struct hostent *hostname = gethostbyname(data->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed with %s\n", data->server.ip);
        pthread_exit(NULL);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->server.port);
    
    // ИСПРАВЛЕННАЯ СТРОКА: используем h_addr_list вместо h_addr
    server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed!\n");
        pthread_exit(NULL);
    }

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", data->server.ip, data->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    // Подготавливаем и отправляем задание
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    // Получаем ответ
    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Recieve failed from %s:%d\n", data->server.ip, data->server.port);
        close(sck);
        pthread_exit(NULL);
    }

    memcpy(&data->result, response, sizeof(uint64_t));
    printf("Received from %s:%d: partial result %lu for range [%lu, %lu]\n", 
           data->server.ip, data->server.port, data->result, data->begin, data->end);

    close(sck);
    return NULL;
}

int ParseServerFile(const char* filename, struct Server** servers) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open servers file: %s\n", filename);
        return -1;
    }

    // Считаем количество строк
    int count = 0;
    char line[255];
    while (fgets(line, sizeof(line), file)) {
        count++;
    }
    
    // Возвращаемся к началу файла
    fseek(file, 0, SEEK_SET);
    
    // Выделяем память
    *servers = malloc(sizeof(struct Server) * count);
    
    // Читаем серверы
    int i = 0;
    while (fgets(line, sizeof(line), file)) {
        // Убираем символ новой строки
        line[strcspn(line, "\n")] = 0;
        
        // Парсим ip:port
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            strcpy((*servers)[i].ip, line);
            (*servers)[i].port = atoi(colon + 1);
            i++;
        }
    }
    
    fclose(file);
    return count;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // 255 - стандартный максимальный путь к файлу в Linux

  while (true) {

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == UINT64_MAX || mod == UINT64_MAX || strlen(servers) == 0) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // Читаем серверы из файла
  struct Server *to = NULL;
  int servers_num = ParseServerFile(servers, &to);
  if (servers_num <= 0) {
    fprintf(stderr, "No servers found or error reading file\n");
    return 1;
  }

  printf("Found %d servers\n", servers_num);

  // Создаем потоки для каждого сервера
  pthread_t threads[servers_num];
  struct ServerThreadData thread_data[servers_num];

  // Распределяем диапазоны между серверами
  uint64_t numbers_per_server = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current = 1;

  for (int i = 0; i < servers_num; i++) {
    thread_data[i].server = to[i];
    thread_data[i].begin = current;
    thread_data[i].end = current + numbers_per_server - 1;
    
    // Распределяем остаток по первым серверам
    if ((uint64_t)i < remainder) {
        thread_data[i].end++;
    }
    
    thread_data[i].mod = mod;
    thread_data[i].result = 1;
    
    printf("Server %d (%s:%d): numbers from %lu to %lu\n", 
           i, to[i].ip, to[i].port, thread_data[i].begin, thread_data[i].end);
    
    if (pthread_create(&threads[i], NULL, ConnectToServer, &thread_data[i]) != 0) {
        fprintf(stderr, "Error creating thread for server %d\n", i);
        free(to);
        return 1;
    }
    
    current = thread_data[i].end + 1;
  }

  // Ждем завершения всех потоков
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
  }

  // Объединяем результаты
  uint64_t total = 1;
  for (int i = 0; i < servers_num; i++) {
    total = MultModulo(total, thread_data[i].result, mod);
  }

  printf("\nFinal result: %lu! mod %lu = %lu\n", k, mod, total);

  free(to);
  return 0;
}