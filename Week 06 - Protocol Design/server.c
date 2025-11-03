#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5500
#define MAX_CLIENTS 10
#define BUF_SIZE 1024

typedef struct {
    int sock;
    char username[50];
    int logged_in;
    FILE *log_file;
} Client;

Client clients[MAX_CLIENTS];

void handle_client_message(Client *client, char *msg);
void send_message(int sock, const char *message);

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUF_SIZE];

    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // Khởi tạo danh sách client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = -1;
        clients[i].logged_in = 0;
        clients[i].username[0] = '\0';
        clients[i].log_file = NULL;
    }

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        // Thêm các socket client vào tập kiểm tra
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sock;
            if (sd >= 0 && sd < FD_SETSIZE) {
                FD_SET(sd, &readfds);
                if (sd > max_sd) max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            continue;
        }

        // Kiểm tra kết nối mới
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (new_socket < 0) {
                perror("accept error");
                continue;
            }

            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == -1) {
                    clients[i].sock = new_socket;
                    clients[i].logged_in = 0;
                    clients[i].username[0] = '\0';
                    clients[i].log_file = NULL;
                    added = 1;
                    printf("New connection accepted: fd=%d\n", new_socket);
                    break;
                }
            }

            if (!added) {
                printf("Too many clients! Closing new connection.\n");
                close(new_socket);
            }
        }

        // Xử lý dữ liệu từ các client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sock;
            if (sd >= 0 && FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUF_SIZE - 1);
                if (valread <= 0) {
                    // Client ngắt kết nối
                    close(sd);
                    if (clients[i].log_file) fclose(clients[i].log_file);
                    clients[i].sock = -1;
                    clients[i].logged_in = 0;
                    clients[i].username[0] = '\0';
                    clients[i].log_file = NULL;
                    printf("Client disconnected.\n");
                } else {
                    buffer[valread] = '\0';
                    handle_client_message(&clients[i], buffer);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}

void handle_client_message(Client *client, char *msg) {
    if (strncmp(msg, "LOGIN", 5) == 0) {
        char username[50];
        if (sscanf(msg, "LOGIN username=%49s", username) != 1) {
            send_message(client->sock, "ERR reason=Invalid_login_format\n");
            return;
        }

        strcpy(client->username, username);
        client->logged_in = 1;

        char filename[64];
        sprintf(filename, "%s.log", username);
        client->log_file = fopen(filename, "a");
        if (!client->log_file) {
            send_message(client->sock, "ERR reason=Cannot_open_logfile\n");
            return;
        }

        send_message(client->sock, "ACK status=OK\n");
        printf("[SERVER] %s logged in.\n", username);
    } 
    else if (strncmp(msg, "TEXT", 4) == 0) {
        if (!client->logged_in) {
            send_message(client->sock, "ERR reason=Not_logged_in\n");
            return;
        }

        char from[50], text[BUF_SIZE];
        if (sscanf(msg, "TEXT from=%49s msg=\"%[^\"]\"", from, text) != 2) {
            send_message(client->sock, "ERR reason=Invalid_text_format\n");
            return;
        }

        if (strcmp(from, client->username) != 0) {
            send_message(client->sock, "ERR reason=Username_mismatch\n");
            return;
        }

        if (client->log_file) {
            fprintf(client->log_file, "%s: %s\n", from, text);
            fflush(client->log_file);
        }

        send_message(client->sock, "ACK status=OK\n");
        printf("[SERVER LOG] %s: %s\n", from, text);
    }
    else {
        send_message(client->sock, "ERR reason=Invalid_message\n");
    }
}

void send_message(int sock, const char *message) {
    if (sock >= 0) {
        send(sock, message, strlen(message), 0);
    }
}
