// stnc.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAXLINE 1024

void run_server(char* port) {
    int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_sockfd, 1);

    char sendline[MAXLINE], recvline[MAXLINE];

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);

    struct pollfd pfd[2];
    pfd[0].fd = client_sockfd;
    pfd[0].events = POLLIN;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = POLLIN;

    while (1) {
        poll(pfd, 2, -1);

        if (pfd[0].revents & POLLIN) {
            int n = read(client_sockfd, recvline, MAXLINE);
            if (n == 0) {
                break;
            }
            printf("Client: %s", recvline);
        }

        if (pfd[1].revents & POLLIN) {
            fgets(sendline, MAXLINE, stdin);
            write(client_sockfd, sendline, strlen(sendline));
        }
    }

    close(client_sockfd);
    close(server_sockfd);
}

void run_client(char* ip, char* port) {
    int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    connect(client_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    char sendline[MAXLINE], recvline[MAXLINE];

    struct pollfd pfd[2];
    pfd[0].fd = client_sockfd;
    pfd[0].events = POLLIN;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = POLLIN;

    while (1) {
        poll(pfd, 2, -1);

        if (pfd[0].revents & POLLIN) {
            int n = read(client_sockfd, recvline, MAXLINE);
            if (n == 0) {
                break;
            }
            printf("Server: %s", recvline);
        }

        if (pfd[1].revents & POLLIN) {
            fgets(sendline, MAXLINE, stdin);
            write(client_sockfd, sendline, strlen(sendline));
        }
    }

    close(client_sockfd);
}
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("Server: %s -s <port>\n", argv[0]);
        printf("Client: %s -c <ip> <port>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-s") == 0 && argc == 3) {
        run_server(argv[2]);
    } else if (strcmp(argv[1], "-c") == 0 && argc == 4) {
        run_client(argv[2], argv[3]);
    } else {
        printf("Invalid arguments.\n");
        printf("Usage:\n");
        printf("Server: %s -s <port>\n", argv[0]);
        printf("Client: %s -c <ip> <port>\n", argv[0]);
        exit(1);
    }

    return 0;
}