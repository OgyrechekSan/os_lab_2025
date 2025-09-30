#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>


#define SADDR struct sockaddr

void print_usage(const char *program_name) {
    printf("Usage: %s --bufsize <size> --port <port>\n", program_name);
    printf("Example: %s --bufsize 100 --port 8080\n", program_name);
}

int main(int argc, char *argv[]) {
    int bufsize = 100;
    int port = 8080;
    
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
    
    printf("Starting TCP server: port=%d, bufsize=%d\n", port, bufsize);
    
    int lfd, cfd;
    int nread;
    char *buf = malloc(bufsize);
    if (!buf) {
        perror("malloc");
        exit(1);
    }
    
    struct sockaddr_in servaddr, cliaddr;
    socklen_t servaddr_len = sizeof(struct sockaddr_in);
    socklen_t cliaddr_len = sizeof(struct sockaddr_in);
    
    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        free(buf);
        exit(1);
    }
    
    memset(&servaddr, 0, servaddr_len);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    if (bind(lfd, (SADDR *)&servaddr, servaddr_len) < 0) {
        perror("bind");
        free(buf);
        close(lfd);
        exit(1);
    }
    
    if (listen(lfd, 5) < 0) {
        perror("listen");
        free(buf);
        close(lfd);
        exit(1);
    }
    
    printf("Server listening on port %d\n", port);
    
    while (1) {
        cliaddr_len = sizeof(struct sockaddr_in);
        if ((cfd = accept(lfd, (SADDR *)&cliaddr, &cliaddr_len)) < 0) {
            perror("accept");
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Connection established from %s:%d\n", client_ip, ntohs(cliaddr.sin_port));
        
        while ((nread = read(cfd, buf, bufsize)) > 0) {
            write(1, buf, nread);
        }
        
        if (nread == -1) {
            perror("read");
        }
        
        printf("Connection closed\n");
        close(cfd);
    }
    
    free(buf);
    close(lfd);
    return 0;
}