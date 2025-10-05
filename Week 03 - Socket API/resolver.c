#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>

#define LOG_FILE "resolver.log"

/// Kiem tra xem chuoi co phai dia chi IPv4/IPv6 hop le 
int is_valid_ip(const char *ip) {
    struct in_addr ipv4;
    struct in6_addr ipv6;
    if (inet_pton(AF_INET, ip, &ipv4) == 1) return 4;
    if (inet_pton(AF_INET6, ip, &ipv6) == 1) return 6;
    return 0;
}

/// Kiem tra dia chi IP dac biet (loopback, private, multicast)
int is_special_ip(const char *ip) {
    struct in_addr ipv4;
    if (inet_pton(AF_INET, ip, &ipv4) == 1) {
        unsigned long addr = ntohl(ipv4.s_addr);
        if ((addr >> 24) == 127) return 1; 
        if ((addr >> 24) == 10) return 1;  
        if ((addr >> 20) == 0xAC1) return 1; 
        if ((addr >> 16) == 0xC0A8) return 1; 
        if ((addr >= 0xE0000000 && addr <= 0xEFFFFFFF)) return 1; 
    }
    if (strstr(ip, "::1") || strncmp(ip, "fc", 2) == 0 || strncmp(ip, "fd", 2) == 0)
        return 1;
    return 0;
}

/// Ghi log
void write_log(const char *query, const char *result) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "[%s] Query: %s\nResult:\n%s\n\n", ctime(&now), query, result);
    fclose(f);
}

/// Tra cuu tu IP → Domain
void resolve_ip(const char *ip) {
    struct sockaddr_storage sa;
    socklen_t sa_len;
    char host[NI_MAXHOST];
    char result[4096]; result[0] = '\0';

    if (is_special_ip(ip)) {
        printf("special IP address — may not have DNS record\n");
    }

    int family = is_valid_ip(ip);
    if (family == 4) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&sa;
        addr4->sin_family = AF_INET;
        inet_pton(AF_INET, ip, &(addr4->sin_addr));
        sa_len = sizeof(struct sockaddr_in);
    } else if (family == 6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&sa;
        addr6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, ip, &(addr6->sin6_addr));
        sa_len = sizeof(struct sockaddr_in6);
    } else {
        printf("Invalid address\n");
        return;
    }

    int ret = getnameinfo((struct sockaddr *)&sa, sa_len, host, sizeof(host),
                          NULL, 0, NI_NAMEREQD);
    if (ret == 0) {
        printf("Official name: %s\n", host);
        snprintf(result, sizeof(result), "Official name: %s", host);
    } else {
        printf("Not found information\n");
        snprintf(result, sizeof(result), "Not found information");
    }

    write_log(ip, result);
}

/// Tra cuu tu Domain → IP
void resolve_domain(const char *domain) {
    struct addrinfo hints, *res, *p;
    char ipstr[INET6_ADDRSTRLEN];
    char result[4096]; result[0] = '\0';
    clock_t start = clock();

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(domain, NULL, &hints, &res);
    if (status != 0) {
        printf("Not found information\n");
        snprintf(result, sizeof(result), "Not found information");
        write_log(domain, result);
        return;
    }

    printf("Official IP: ");
    inet_ntop(res->ai_family,
              res->ai_family == AF_INET ?
              (void *)&((struct sockaddr_in *)res->ai_addr)->sin_addr :
              (void *)&((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,
              ipstr, sizeof ipstr);
    printf("%s\n", ipstr);
    snprintf(result + strlen(result), sizeof(result)-strlen(result), "Official IP: %s\n", ipstr);

    printf("Alias IP:\n");
    for (p = res->ai_next; p != NULL; p = p->ai_next) {
        void *addr;
        if (p->ai_family == AF_INET) {
            addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
        } else {
            addr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
        }
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("%s\n", ipstr);
        snprintf(result + strlen(result), sizeof(result)-strlen(result), "%s\n", ipstr);
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Query time: %.6f seconds\n", elapsed);

    write_log(domain, result);
    freeaddrinfo(res);
}

/// Xu ly 1 truy van (domain hoac ip)
void handle_query(const char *input) {
    if (strlen(input) == 0) return;
    if (is_valid_ip(input)) {
        resolve_ip(input);
    } else {
        resolve_domain(input);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 2) {
        if (strstr(argv[1], ".txt")) {
            FILE *f = fopen(argv[1], "r");
            if (!f) {
                perror("Cannot open file");
                return 1;
            }
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = 0;
                handle_query(line);
                printf("\n");
            }
            fclose(f);
        } else {
            handle_query(argv[1]);
        }
    } else if (argc == 1) {
        char line[256];
        printf("Enter domain or IP (empty line to quit):\n");
        while (1) {
            printf("> ");
            if (!fgets(line, sizeof(line), stdin)) break;
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0) break;

            char *token = strtok(line, " ");
            while (token) {
                handle_query(token);
                printf("\n");
                token = strtok(NULL, " ");
            }
        }
    } else {
        printf("Usage: %s [domain|ip|file.txt]\n", argv[0]);
    }
    return 0;
}
