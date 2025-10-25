#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 1024

ssize_t send_line(int sockfd, const char *buf) {
    char tmp[MAXLINE + 2];
    size_t len = strlen(buf);
    if (len >= MAXLINE) len = MAXLINE - 1;
    memcpy(tmp, buf, len);
    tmp[len++] = '\n';
    tmp[len] = '\0';
    return send(sockfd, tmp, len, 0);
}

ssize_t recv_line(int sockfd, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;
    while (i + 1 < maxlen) {
        ssize_t n = recv(sockfd, &c, 1, 0);
        if (n <= 0) return n;
        if (c == '\n' || c == '\r') break;
        buf[i++] = c;
    }
    buf[i] = 0;
    return i;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ServerIP> <Port>\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    char sendbuf[MAXLINE], recvbuf[MAXLINE];

    while (1) {
        if (recv_line(sockfd, recvbuf, sizeof(recvbuf)) <= 0) break;
        printf("Server: %s\n", recvbuf);

        if (strstr(recvbuf, "Username") != NULL) {
            printf("Username (empty to quit): ");
            fflush(stdout);
            if (!fgets(sendbuf, sizeof(sendbuf), stdin)) break;
            sendbuf[strcspn(sendbuf, "\r\n")] = 0;
            send_line(sockfd, sendbuf);
            if (strlen(sendbuf) == 0) break;
            continue;
        }

        if (strstr(recvbuf, "Insert password") != NULL) {
            int blocked = 0;
            while (1) {
                printf("Password: ");
                fflush(stdout);
                if (!fgets(sendbuf, sizeof(sendbuf), stdin)) break;
                sendbuf[strcspn(sendbuf, "\r\n")] = 0;
                send_line(sockfd, sendbuf);

                if (recv_line(sockfd, recvbuf, sizeof(recvbuf)) <= 0) break;
                printf("Server: %s\n", recvbuf);

                if (strcmp(recvbuf, "Not OK") == 0)
                    continue;
                else if (strcmp(recvbuf, "Account is blocked") == 0) {
                    blocked = 1;
                    break;
                } else if (strstr(recvbuf, "Hello") != NULL)
                    break;
            }
            if (blocked) continue;
        }

        if (strstr(recvbuf, "Hello") != NULL) {
            while (1) {
                printf("Enter command (any alnum string | homepage | bye): ");
                fflush(stdout);
                if (!fgets(sendbuf, sizeof(sendbuf), stdin)) break;
                sendbuf[strcspn(sendbuf, "\r\n")] = 0;
                send_line(sockfd, sendbuf);
                if (strlen(sendbuf) == 0) break;
                if (recv_line(sockfd, recvbuf, sizeof(recvbuf)) <= 0) break;
                printf("Server: %s\n", recvbuf);
                if (strncmp(recvbuf, "Goodbye", 7) == 0)
                    break;
            }
        }
    }

    printf("Server closed\n");
    close(sockfd);
    return 0;
}
