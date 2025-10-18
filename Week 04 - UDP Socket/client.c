#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ServerIP> <PortNumber>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t len;
    char buffer[MAX];
    char input[MAX];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    len = sizeof(servaddr);

    while (1) {
        printf("Enter username (empty to quit): ");
        fgets(input, MAX, stdin);
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) break;

        sendto(sockfd, input, strlen(input), 0, (struct sockaddr *)&servaddr, len);
        int recv_len = recvfrom(sockfd, buffer, MAX, 0, (struct sockaddr *)&servaddr, &len);
        buffer[recv_len] = '\0';
        printf("%s\n", buffer);

        fgets(input, MAX, stdin);
        input[strcspn(input, "\n")] = '\0';
        sendto(sockfd, input, strlen(input), 0, (struct sockaddr *)&servaddr, len);

        recv_len = recvfrom(sockfd, buffer, MAX, 0, (struct sockaddr *)&servaddr, &len);
        buffer[recv_len] = '\0';
        printf("%s\n", buffer);

        if (strcmp(buffer, "OK") == 0) {
            while (1) {
                printf("Enter command (new password / homepage / bye): ");
                fgets(input, MAX, stdin);
                input[strcspn(input, "\n")] = '\0';
                if (strlen(input) == 0) break;

                sendto(sockfd, input, strlen(input), 0, (struct sockaddr *)&servaddr, len);
                recv_len = recvfrom(sockfd, buffer, MAX, 0, (struct sockaddr *)&servaddr, &len);
                buffer[recv_len] = '\0';
                printf("%s\n", buffer);

                if (strncmp(buffer, "Goodbye", 7) == 0) break;
            }
        }
    }

    close(sockfd);
    return 0;
}
