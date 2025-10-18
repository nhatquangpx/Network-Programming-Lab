#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAX 1024
#define ACCOUNT_FILE "account.txt"

typedef struct {
    char username[64];
    char password[64];
    char email[128];
    char homepage[128];
    int status; 
} Account;

int loadAccounts(Account *accounts) {
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f) {
        perror("Cannot open account file");
        exit(1);
    }
    int count = 0;
    while (fscanf(f, "%s %s %s %s %d",
                  accounts[count].username,
                  accounts[count].password,
                  accounts[count].email,
                  accounts[count].homepage,
                  &accounts[count].status) == 5) {
        count++;
    }
    fclose(f);
    return count;
}

void saveAccounts(Account *accounts, int n) {
    FILE *f = fopen(ACCOUNT_FILE, "w");
    for (int i = 0; i < n; i++) {
        fprintf(f, "%s %s %s %s %d\n",
                accounts[i].username,
                accounts[i].password,
                accounts[i].email,
                accounts[i].homepage,
                accounts[i].status);
    }
    fclose(f);
}

int isValidPassword(const char *pwd) {
    for (int i = 0; i < strlen(pwd); i++) {
        if (!isalnum(pwd[i])) return 0;
    }
    return 1;
}

void extractLettersDigits(const char *pwd, char *letters, char *digits) {
    int l = 0, d = 0;
    for (int i = 0; i < strlen(pwd); i++) {
        if (isalpha(pwd[i])) letters[l++] = pwd[i];
        else if (isdigit(pwd[i])) digits[d++] = pwd[i];
    }
    letters[l] = '\0';
    digits[d] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <PortNumber>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buffer[MAX];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    printf("Server running on port %d...\n", port);

    Account accounts[100];
    int n = loadAccounts(accounts);

    char username[64], password[64];
    int failCount = 0, loggedIn = 0, idx = -1;

    while (1) {
        len = sizeof(cliaddr);
        int recv_len = recvfrom(sockfd, buffer, MAX, 0, (struct sockaddr *)&cliaddr, &len);
        buffer[recv_len] = '\0';

        if (!loggedIn) {
            strcpy(username, buffer);
            sendto(sockfd, "Insert password", strlen("Insert password"), 0, (struct sockaddr *)&cliaddr, len);

            recv_len = recvfrom(sockfd, password, MAX, 0, (struct sockaddr *)&cliaddr, &len);
            password[recv_len] = '\0';

            idx = -1;
            for (int i = 0; i < n; i++) {
                if (strcmp(accounts[i].username, username) == 0) {
                    idx = i;
                    break;
                }
            }

            if (idx == -1) {
                sendto(sockfd, "Account not found", strlen("Account not found"), 0, (struct sockaddr *)&cliaddr, len);
                continue;
            }

            if (accounts[idx].status == 0) {
                sendto(sockfd, "account not ready", strlen("account not ready"), 0, (struct sockaddr *)&cliaddr, len);
                continue;
            }

            if (strcmp(accounts[idx].password, password) == 0) {
                loggedIn = 1;
                failCount = 0;
                sendto(sockfd, "OK", strlen("OK"), 0, (struct sockaddr *)&cliaddr, len);
                printf("User %s logged in.\n", username);
            } else {
                failCount++;
                if (failCount >= 3) {
                    accounts[idx].status = 0;
                    saveAccounts(accounts, n);
                    sendto(sockfd, "Account is blocked", strlen("Account is blocked"), 0, (struct sockaddr *)&cliaddr, len);
                } else {
                    sendto(sockfd, "not OK", strlen("not OK"), 0, (struct sockaddr *)&cliaddr, len);
                }
            }
        } else {
            if (strcmp(buffer, "bye") == 0) {
                char msg[100];
                sprintf(msg, "Goodbye %s", username);
                sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&cliaddr, len);
                loggedIn = 0;
                printf("User %s logged out.\n", username);
            } else if (strcmp(buffer, "homepage") == 0) {
                sendto(sockfd, accounts[idx].homepage, strlen(accounts[idx].homepage), 0, (struct sockaddr *)&cliaddr, len);
            } else {
                if (!isValidPassword(buffer)) {
                    sendto(sockfd, "Error", strlen("Error"), 0, (struct sockaddr *)&cliaddr, len);
                } else {
                    char letters[64], digits[64], msg[128];
                    extractLettersDigits(buffer, letters, digits);
                    sprintf(msg, "%s\n%s", digits, letters);
                    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&cliaddr, len);
                    strcpy(accounts[idx].password, buffer);
                    saveAccounts(accounts, n);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
