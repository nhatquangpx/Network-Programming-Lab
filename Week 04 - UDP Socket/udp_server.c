#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERV_PORT 8080
#define BUFF_SIZE 1024

int main() {
    int sockfd;
    char buff[BUFF_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    int rcvBytes, sendBytes;

    // Tao socket UDP 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Cau hinh dia chi server
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERV_PORT);

    // Gan dia chi cho socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d...\n", SERV_PORT);

    // Nhan du lieu tu client va gui phan hoi
    while (1) {
        len = sizeof(cliaddr);
        rcvBytes = recvfrom(sockfd, (char *)buff, BUFF_SIZE, 0,
                            (struct sockaddr *)&cliaddr, &len);
        if (rcvBytes < 0) {
            perror("recvfrom failed");
            continue;
        }

        buff[rcvBytes] = '\0';
        printf("Received from [%s:%d]: %s\n", inet_ntoa(cliaddr.sin_addr),
               ntohs(cliaddr.sin_port), buff);

        char reply[BUFF_SIZE];
        snprintf(reply, sizeof(reply), "Hello %s:%d from %s:%d",
                 inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port),
                 inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));

        sendBytes = sendto(sockfd, reply, strlen(reply), 0,
                           (struct sockaddr *)&cliaddr, len);
        if (sendBytes < 0) {
            perror("sendto failed");
        }
    }

    close(sockfd);
    return 0;
}
