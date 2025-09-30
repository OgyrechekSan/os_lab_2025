#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr

void print_usage(const char *program_name) {
    printf("Usage: %s --bufsize <size> --ip <ip> --port <port>\n", program_name);
    printf("Example: %s --bufsize 1024 --ip 127.0.0.1 --port 20001\n", program_name);
}

int main(int argc, char **argv) {
    int bufsize = 1024;
    char ip[16] = "127.0.0.1";
    int port = 20001;
    
    // Парсинг аргументов командной строки
    static struct option long_options[] = {
        {"bufsize", required_argument, 0, 'b'},
        {"ip", required_argument, 0, 'i'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "b:i:p:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'b':
                bufsize = atoi(optarg);
                if (bufsize <= 0) {
                    fprintf(stderr, "Buffer size must be positive\n");
                    exit(1);
                }
                break;
            case 'i':
                strncpy(ip, optarg, sizeof(ip) - 1);
                ip[sizeof(ip) - 1] = '\0';
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
    
    printf("Starting UDP client: IP=%s, port=%d, bufsize=%d\n", ip, port, bufsize);
    
    int sockfd, n;
    char *sendline = malloc(bufsize);
    char *recvline = malloc(bufsize + 1);
    if (!sendline || !recvline) {
        perror("malloc");
        free(sendline);
        free(recvline);
        exit(1);
    }
    
    struct sockaddr_in servaddr;
    socklen_t servaddr_len = sizeof(struct sockaddr_in);
    
    memset(&servaddr, 0, servaddr_len);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0) {
        perror("inet_pton problem");
        free(sendline);
        free(recvline);
        exit(1);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(sendline);
        free(recvline);
        exit(1);
    }
    
    printf("Enter string (Ctrl+D to exit):\n");
    
    while ((n = read(0, sendline, bufsize)) > 0) {
        if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, servaddr_len) == -1) {
            perror("sendto problem");
            break;
        }
        
        if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
            perror("recvfrom problem");
            break;
        }
        
        recvline[n] = '\0';
        printf("REPLY FROM SERVER: %s\n", recvline);
    }
    
    free(sendline);
    free(recvline);
    close(sockfd);
    return 0;
}