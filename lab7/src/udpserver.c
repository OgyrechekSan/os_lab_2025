#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr

void print_usage(const char *program_name) {
    printf("Usage: %s --bufsize <size> --port <port>\n", program_name);
    printf("Example: %s --bufsize 1024 --port 20001\n", program_name);
}

int main(int argc, char *argv[]) {
    int bufsize = 1024;
    int port = 20001;
    
    // Парсинг аргументов командной строки
    static struct option long_options[] = {
        {"bufsize", required_argument, 0, 'b'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "b:p:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'b':
                bufsize = atoi(optarg);
                if (bufsize <= 0) {
                    fprintf(stderr, "Buffer size must be positive\n");
                    exit(1);
                }
                break;
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Port must be between 1 and 65535\n");
                    exit(1);
                }
                break;
            default:
                print_usage(argv[0]);
                exit(1);
        }
    }
    
    if (optind < argc) {
        printf("Non-option arguments: ");
        while (optind < argc) {
            printf("%s ", argv[optind++]);
        }
        printf("\n");
        print_usage(argv[0]);
        exit(1);
    }
    
    printf("Starting UDP server: port=%d, bufsize=%d\n", port, bufsize);
    
    int sockfd, n;
    char *mesg = malloc(bufsize);
    char ipadr[16];
    if (!mesg) {
        perror("malloc");
        exit(1);
    }
    
    struct sockaddr_in servaddr, cliaddr;
    socklen_t servaddr_len = sizeof(struct sockaddr_in);
    socklen_t cliaddr_len = sizeof(struct sockaddr_in);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(mesg);
        exit(1);
    }
    
    memset(&servaddr, 0, servaddr_len);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    if (bind(sockfd, (SADDR *)&servaddr, servaddr_len) < 0) {
        perror("bind problem");
        free(mesg);
        close(sockfd);
        exit(1);
    }
    
    printf("SERVER starts on port %d...\n", port);
    
    while (1) {
        cliaddr_len = sizeof(struct sockaddr_in);
        if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &cliaddr_len)) < 0) {
            perror("recvfrom");
            continue;
        }
        mesg[n] = '\0';
        
        printf("REQUEST: '%s' FROM %s:%d\n", mesg,
               inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
               ntohs(cliaddr.sin_port));
        
        if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, cliaddr_len) < 0) {
            perror("sendto");
            continue;
        }
    }
    
    free(mesg);
    close(sockfd);
    return 0;
}