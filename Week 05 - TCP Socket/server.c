#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXLINE 1024
#define ACCOUNT_FILE "account.txt"
#define MAX_ACCOUNTS 256

typedef struct {
    char username[64];
    char password[128];
    char email[128];
    char homepage[256];
    int status; 
} Account;

Account accounts[MAX_ACCOUNTS];
int account_count = 0;

int load_accounts() {
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f) return -1;
    account_count = 0;
    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        if (buf[0] == '#' || strlen(buf) < 2) continue;
        buf[strcspn(buf, "\n")] = 0;

        char u[64], pw[128], em[128], hp[256];
        int st = 1;
        int n = sscanf(buf, "%63s %127s %127s %255s %d", u, pw, em, hp, &st);
        if (n >= 2) {
            strncpy(accounts[account_count].username, u, sizeof(accounts[account_count].username) - 1);
            accounts[account_count].username[sizeof(accounts[account_count].username) - 1] = '\0';
            strncpy(accounts[account_count].password, pw, sizeof(accounts[account_count].password) - 1);
            accounts[account_count].password[sizeof(accounts[account_count].password) - 1] = '\0';
            if (n >= 3) {
                strncpy(accounts[account_count].email, em, sizeof(accounts[account_count].email) - 1);
                accounts[account_count].email[sizeof(accounts[account_count].email) - 1] = '\0';
            } else accounts[account_count].email[0] = '\0';
            if (n >= 4) {
                strncpy(accounts[account_count].homepage, hp, sizeof(accounts[account_count].homepage) - 1);
                accounts[account_count].homepage[sizeof(accounts[account_count].homepage) - 1] = '\0';
            } else accounts[account_count].homepage[0] = '\0';
            accounts[account_count].status = st;
            account_count++;
            if (account_count >= MAX_ACCOUNTS) break;
        }
    }
    fclose(f);
    return 0;
}

int save_accounts() {
    FILE *f = fopen(ACCOUNT_FILE, "w");
    if (!f) return -1;
    for (int i = 0; i < account_count; i++) {
        fprintf(f, "%s %s %s %s %d\n",
                accounts[i].username,
                accounts[i].password,
                accounts[i].email[0] ? accounts[i].email : "none",
                accounts[i].homepage[0] ? accounts[i].homepage : "none",
                accounts[i].status);
    }
    fclose(f);
    return 0;
}

int find_account_index(const char *username) {
    for (int i = 0; i < account_count; i++) {
        if (strcmp(accounts[i].username, username) == 0) return i;
    }
    return -1;
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

ssize_t send_line(int sockfd, const char *buf) {
    char tmp[MAXLINE + 2];
    size_t len = strlen(buf);
    if (len >= MAXLINE) len = MAXLINE - 1;
    memcpy(tmp, buf, len);
    tmp[len++] = '\n';
    tmp[len] = '\0';
    return send(sockfd, tmp, len, 0);
}

int is_alnum_str(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; ++s)
        if (!isalnum((unsigned char)*s))
            return 0;
    return 1;
}

void split_letters_digits(const char *s, char *letters, char *digits) {
    letters[0] = digits[0] = '\0';
    while (*s) {
        if (isalpha((unsigned char)*s)) {
            size_t l = strlen(letters);
            if (l < 255) letters[l++] = *s, letters[l] = '\0';
        } else if (isdigit((unsigned char)*s)) {
            size_t d = strlen(digits);
            if (d < 255) digits[d++] = *s, digits[d] = '\0';
        }
        s++;
    }
}

int handle_session(int connfd, int idx) {
    char buf[MAXLINE];
    char username[64];
    strncpy(username, accounts[idx].username, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';

    char welcome[128];
    snprintf(welcome, sizeof(welcome), "Hello %s", username);
    send_line(connfd, welcome);

    while (1) {
        ssize_t r = recv_line(connfd, buf, sizeof(buf));
        if (r <= 0) return -1;
        if (strlen(buf) == 0) break;

        if (strcmp(buf, "bye") == 0) {
            char g[128];
            snprintf(g, sizeof(g), "Goodbye %s", username);
            send_line(connfd, g);
            return 1;
        } else if (strcmp(buf, "homepage") == 0) {
            if (accounts[idx].homepage[0])
                send_line(connfd, accounts[idx].homepage);
            else
                send_line(connfd, "No homepage set");
        } else if (is_alnum_str(buf)) {
            strncpy(accounts[idx].password, buf, sizeof(accounts[idx].password) - 1);
            accounts[idx].password[sizeof(accounts[idx].password) - 1] = '\0';
            save_accounts();
            char letters[256] = {0}, digits[256] = {0};
            split_letters_digits(buf, letters, digits);
            char resp[512];
            snprintf(resp, sizeof(resp), "Password changed: %s %s", letters, digits);
            send_line(connfd, resp);
        } else {
            send_line(connfd, "Error");
        }
    }
    return 0;
}

int handle_client(int connfd) {
    char buf[MAXLINE];
    ssize_t r;

    while (1) {
        send_line(connfd, "Username (empty to quit):");
        r = recv_line(connfd, buf, sizeof(buf));
        if (r <= 0) return -1;
        if (strlen(buf) == 0) return 0;

        char username[64];
        strncpy(username, buf, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
        int idx = find_account_index(username);
        if (idx < 0) {
            send_line(connfd, "User not found");
            continue;
        }

        if (accounts[idx].status == 0) {
            send_line(connfd, "account not ready");
            continue;
        }

        send_line(connfd, "Insert password");
        int attempts = 0;
        while (1) {
            r = recv_line(connfd, buf, sizeof(buf));
            if (r <= 0) return -1;
            if (strcmp(buf, accounts[idx].password) == 0) {
                break;
            } else {
                attempts++;
                if (attempts >= 3) {
                    accounts[idx].status = 0;
                    save_accounts();
                    send_line(connfd, "Account is blocked");
                    idx = -1;
                    break;
                } else {
                    send_line(connfd, "Not OK");
                }
            }
        }
        if (idx < 0) continue;

        int res = handle_session(connfd, idx);
        if (res == 1)
            continue; 
        else
            break;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        exit(1);
    }

    if (load_accounts() != 0)
        fprintf(stderr, "Warning: cannot open account file %s\n", ACCOUNT_FILE);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }

    int port = atoi(argv[1]);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        if (connfd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected\n");
        handle_client(connfd);
        close(connfd);
        printf("Client disconnected\n");
    }

    close(listenfd);
    return 0;
}