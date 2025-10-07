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
    struct sockaddr_in servaddr;
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
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    // Gui du lieu den server
    strcpy(buff, "Hello world");
    len = sizeof(servaddr);

    sendBytes = sendto(sockfd, buff, strlen(buff), 0,
                       (struct sockaddr *)&servaddr, len);
    if (sendBytes < 0) {
        perror("sendto failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Nhan response tu server
    rcvBytes = recvfrom(sockfd, buff, BUFF_SIZE, 0,
                        (struct sockaddr *)&servaddr, &len);
    if (rcvBytes < 0) {
        perror("recvfrom failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    buff[rcvBytes] = '\0';
    printf("Message from server: %s\n", buff);

    close(sockfd);
    return 0;
}
