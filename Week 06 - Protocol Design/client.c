#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5500
#define BUF_SIZE 1024

void clear_buffer(char *buf) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE], msg[BUF_SIZE];
    char username[50];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to server.\n");

    while (1) {
        // ======= BẮT ĐẦU MỘT PHIÊN LOGIN MỚI =======
        printf("\nEnter login name (empty to quit): ");
        fgets(username, sizeof(username), stdin);
        username[strcspn(username, "\n")] = 0;

        if (strlen(username) == 0) {
            printf("Goodbye!\n");
            break; // kết thúc chương trình
        }

        sprintf(msg, "LOGIN username=%s", username);
        send(sock, msg, strlen(msg), 0);

        int valread = read(sock, buffer, BUF_SIZE - 1);
        if (valread <= 0) {
            printf("Connection lost.\n");
            break;
        }
        buffer[valread] = '\0';
        printf("Server: %s\n", buffer);

        if (strncmp(buffer, "ACK", 3) != 0) {
            printf("Login failed, try again.\n");
            continue;
        }

        printf("✅ Logged in as '%s'. Type 'logout' to return to login screen.\n", username);

        // ======= VÒNG GỬI TIN NHẮN SAU KHI LOGIN =======
        while (1) {
            printf("Enter message (or 'logout' to re-login): ");
            fgets(msg, sizeof(msg), stdin);
            msg[strcspn(msg, "\n")] = 0;

            if (strcmp(msg, "logout") == 0) {
                printf("Logging out...\n");
                break; // quay lại nhập login name
            }

            if (strlen(msg) == 0) continue; // bỏ qua dòng trống

            char text_msg[BUF_SIZE];
            sprintf(text_msg, "TEXT from=%s msg=\"%s\"", username, msg);
            send(sock, text_msg, strlen(text_msg), 0);

            valread = read(sock, buffer, BUF_SIZE - 1);
            if (valread <= 0) {
                printf("Server disconnected.\n");
                close(sock);
                return 0;
            }

            buffer[valread] = '\0';
            printf("Server: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
