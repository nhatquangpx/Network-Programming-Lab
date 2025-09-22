#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ACCOUNT_FILE "account.txt"
#define HISTORY_FILE "history.txt"
#define MAX_LINE 512
#define LOCK_DURATION 600

typedef struct Account {
    char username[64];
    char password[64];
    char email[128];
    char phone[32];
    int status;
    char role[16];
    long unlock_ts;
    int fail_count;
    struct Account *next;
} Account;

Account *head = NULL;
Account *current_user = NULL;

// Cac ham tien ich

// Ham bo ky tu xuong dong va quay ve dau dong
void trim_newline(char *s) {
    size_t l = strlen(s);
    if (l == 0) return;
    if (s[l-1] == '\n') s[l-1]=0;
    if (l>1 && s[l-2] == '\r') s[l-2] = 0;
}

// Ham lay timestamp hien tai (Epoch)
long now_ts() {
    return (long)time(NULL);
}

// Ham chuyen timestampt (epoch) sang chuoi ngay gio
void epoch_to_str(long ts, char *buf, size_t n) {
    time_t t = (time_t) ts;
    struct tm *tm = localtime(&t);
    strftime(buf, n, "%Y-%m-%d %H:%M:%S", tm);
}

// File I/O cho account
Account* create_account_node() {
    Account *a = (Account*)malloc(sizeof(Account));
    if (!a) {
        perror("malloc");
        exit(1);
    }
    a->username[0]=a->password[0]=a->email[0]=a->phone[0]='\0';
    a->status=1;
    strcpy(a->role,"user");
    a->unlock_ts=0;
    a->fail_count=0;
    a->next=NULL;
    return a;
}

// Dua tai khoan moi vao dau danh sach
void push_account(Account *node){
    node->next = head;
    head = node;
}

// Giai phong toan bo tai khoan
void free_accounts() {
    Account *p = head;
    while(p) {
        Account *n = p->next;
        free(p);
        p = n;
    }
    head = NULL;
}

// Nap file account.txt vao bo nho
void load_accounts() {
    free_accounts();
    FILE *f = fopen(ACCOUNT_FILE, "r");
    if (!f) return;
    char line[MAX_LINE];
    while(fgets(line, sizeof(line),f)){
        trim_newline(line);
        if(strlen(line)==0) continue;
        Account *a = create_account_node();
        char role_buf[32] = {0};
        long unlock = 0;
        int items = sscanf(line, "%63s %63s %127s %31s %d %31s %ld", a->username, a->password, a->email, a->phone, &a->status, role_buf, &unlock);
        if (items>=5) {
            if (items>=6) strncpy(a->role, role_buf, sizeof(a->role)-1);
            else strcpy(a->role, "user");
            if (items>=7) a->unlock_ts = unlock;
            else a->unlock_ts = 0;
            a->fail_count = 0;
            push_account(a);
        } else {
            free(a);
        }
    }
    fclose(f);
}

// Luu tai khoan vao file account.txt
void save_accounts() {
    FILE *f = fopen(ACCOUNT_FILE, "w");
    if (!f) { perror("open account.txt for write"); return; }
    Account *p = head;
    while(p) {
        fprintf(f, "%s %s %s %s %d %s %ld\n",
            p->username, p->password, p->email, p->phone, p->status, p->role, p->unlock_ts);
        p = p->next;
    }
    fclose(f);
}

// Lich su
void append_history(const char *username) {
    FILE *f = fopen(HISTORY_FILE, "a");
    if (!f) { perror("open history.txt"); return; }
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[64], timebuf[64];
    strftime(date, sizeof(date), "%Y-%m-%d", tm);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm);
    fprintf(f, "%s | %s | %s\n", username, date, timebuf);
    fclose(f);
}

// Tim tai khoan
Account* find_account(const char *username) {
    Account *p = head;
    while(p) {
        if (strcmp(p->username, username)==0) return p;
        p = p->next;
    }
    return NULL;
}

// Menu
void print_menu() {
    printf("\nUSER MANAGEMENT PROGRAM\n");
    printf("-----------------------------------\n");
    printf("1. Register\n");
    printf("2. Sign in\n");
    printf("3. Change password\n");
    printf("4. Update account info\n");
    printf("5. Reset password\n");
    printf("6. View login history\n");
    printf("7. Auto-unlock after time\n");
    printf("8. Role & Admin actions\n");
    printf("9. Sign out\n");
    printf("Other to quit\n");
    printf("Your choice (1-9, other to quit): ");
}

// 1. Register
void do_register() {
    char username[64], password[64], email[128], phone[32];
    printf("Enter username: ");
    if (!fgets(username, sizeof(username), stdin)) return;
    trim_newline(username);
    if (strlen(username)==0) { printf("Empty username, cancel.\n"); return; }
    if (find_account(username)) {
        printf("Username already exists.\n");
        return;
    }
    printf("Enter password: ");
    if (!fgets(password, sizeof(password), stdin)) return;
    trim_newline(password);
    printf("Enter email: ");
    if (!fgets(email, sizeof(email), stdin)) return;
    trim_newline(email);
    printf("Enter phone: ");
    if (!fgets(phone, sizeof(phone), stdin)) return;
    trim_newline(phone);

    Account *a = create_account_node();
    strncpy(a->username, username, sizeof(a->username)-1);
    strncpy(a->password, password, sizeof(a->password)-1);
    strncpy(a->email, email, sizeof(a->email)-1);
    strncpy(a->phone, phone, sizeof(a->phone)-1);
    a->status = 1;
    strcpy(a->role, "user");
    a->unlock_ts = 0;
    push_account(a);
    save_accounts();
    printf("Register successful.\n");
}

// Tu dong mo khoa tai khoan khi het thoi gian khoa
void check_and_auto_unlock(Account *a) {
    if (!a) return;
    if (a->status==0 && a->unlock_ts>0) {
        long t = now_ts();
        if (t >= a->unlock_ts) {
            a->status = 1;
            a->unlock_ts = 0;
            a->fail_count = 0;
            save_accounts();
            printf("Account %s automatically unlocked now.\n", a->username);
        }
    }
}

// 2. Sign in
void do_sign_in() {
    char username[64], password[64];
    printf("Enter username: ");
    if (!fgets(username, sizeof(username), stdin)) return;
    trim_newline(username);
    Account *a = find_account(username);
    if (!a) {
        printf("Account does not exist.\n");
        return;
    }
    check_and_auto_unlock(a);
    if (a->status==0) {
        printf("Your account is blocked.\n");
        return;
    }
    printf("Enter password: ");
    if (!fgets(password, sizeof(password), stdin)) return;
    trim_newline(password);
    if (strcmp(a->password, password)==0) {
        current_user = a;
        a->fail_count = 0;
        printf("Welcome %s\n", a->username);
        append_history(a->username);
    } else {
        a->fail_count++;
        printf("Password incorrect.\n");
        if (a->fail_count >= 3) {
            a->status = 0;
            a->unlock_ts = now_ts() + LOCK_DURATION;
            save_accounts();
            printf("Too many failed attempts. Account is blocked for %d seconds.\n", LOCK_DURATION);
        } else {
            printf("You have %d/3 failed attempts.\n", a->fail_count);
        }
    }
}

// 3. Change password
void do_change_password() {
    if (!current_user) { printf("You must sign in first.\n"); return; }
    char oldp[64], newp[64];
    printf("Enter old password: ");
    if (!fgets(oldp, sizeof(oldp), stdin)) return;
    trim_newline(oldp);
    if (strcmp(current_user->password, oldp)!=0) {
        printf("Old password incorrect.\n");
        return;
    }
    printf("Enter new password: ");
    if (!fgets(newp, sizeof(newp), stdin)) return;
    trim_newline(newp);
    strncpy(current_user->password, newp, sizeof(current_user->password)-1);
    save_accounts();
    printf("Password changed.\n");
}

// 4. Update account info
void do_update_info() {
    if (!current_user) { printf("You must sign in first.\n"); return; }
    printf("Update email (enter for skip): ");
    char buf[128];
    if (!fgets(buf, sizeof(buf), stdin)) return;
    trim_newline(buf);
    if (strlen(buf)>0) {
        strncpy(current_user->email, buf, sizeof(current_user->email)-1);
    }
    printf("Update phone (enter for skip): ");
    if (!fgets(buf, sizeof(buf), stdin)) return;
    trim_newline(buf);
    if (strlen(buf)>0) {
        strncpy(current_user->phone, buf, sizeof(current_user->phone)-1);
    }
    save_accounts();
    printf("Account info updated.\n");
}

// 5. Reset password
void do_reset_password() {
    char username[64];
    printf("Enter username to reset: ");
    if (!fgets(username, sizeof(username), stdin)) return;
    trim_newline(username);
    Account *a = find_account(username);
    if (!a) { printf("Account not found.\n"); return; }
    printf("Enter verification code (pretend sent to email): ");
    char code[64];
    if (!fgets(code, sizeof(code), stdin)) return;
    trim_newline(code);
    if (strlen(code)==0) { printf("Invalid code.\n"); return; }
    char newp[64];
    printf("Enter new password: ");
    if (!fgets(newp, sizeof(newp), stdin)) return;
    trim_newline(newp);
    strncpy(a->password, newp, sizeof(a->password)-1);
    a->status = 1;
    a->unlock_ts = 0;
    a->fail_count = 0;
    save_accounts();
    printf("Password reset successful for %s.\n", a->username);
}

// 6. View login history
void do_view_history() {
    if (!current_user) { printf("You must sign in first.\n"); return; }
    FILE *f = fopen(HISTORY_FILE, "r");
    if (!f) { printf("No history available.\n"); return; }
    char line[MAX_LINE];
    printf("Login history for %s:\n", current_user->username);
    int found = 0;
    while(fgets(line, sizeof(line), f)) {
        if (strstr(line, current_user->username) == line) { /* starts with username */
            printf("%s", line);
            found = 1;
        }
    }
    if (!found) printf("No history entries.\n");
    fclose(f);
}

// 7. Auto-unlock: quet het cac tai khoan va tu dong unlock neu het thoi gian khoa
void do_auto_unlock_scan() {
    Account *p = head;
    int any = 0;
    long t = now_ts();
    while(p) {
        if (p->status==0 && p->unlock_ts>0 && t >= p->unlock_ts) {
            p->status = 1;
            p->unlock_ts = 0;
            p->fail_count = 0;
            any = 1;
            printf("Account %s unlocked.\n", p->username);
        }
        p = p->next;
    }
    if (any) {
        save_accounts();
    } else {
        printf("No accounts to unlock now.\n");
    }
}

// Cac ham cua admin
void admin_list_accounts() {
    Account *p = head;
    printf("All accounts:\n");
    while(p) {
        char tsbuf[64]="0";
        if (p->unlock_ts>0) epoch_to_str(p->unlock_ts, tsbuf, sizeof(tsbuf));
        printf("username=%s email=%s phone=%s status=%d role=%s unlock=%s\n",
            p->username, p->email, p->phone, p->status, p->role, tsbuf);
        p = p->next;
    }
}
void admin_delete_account() {
    char username[64];
    printf("Enter username to delete: ");
    if (!fgets(username, sizeof(username), stdin)) return;
    trim_newline(username);
    if (strcmp(username, current_user->username)==0) { printf("Admin cannot delete self.\n"); return; }
    Account *p = head, *prev = NULL;
    while(p) {
        if (strcmp(p->username, username)==0) {
            if (prev) prev->next = p->next;
            else head = p->next;
            free(p);
            save_accounts();
            printf("Account %s deleted.\n", username);
            return;
        }
        prev = p; p = p->next;
    }
    printf("Account not found.\n");
}
void admin_reset_other_password() {
    char username[64];
    printf("Enter username to reset password: ");
    if (!fgets(username, sizeof(username), stdin)) return;
    trim_newline(username);
    Account *a = find_account(username);
    if (!a) { printf("Not found.\n"); return; }
    char newp[64];
    printf("Enter new password: ");
    if (!fgets(newp, sizeof(newp), stdin)) return;
    trim_newline(newp);
    strncpy(a->password, newp, sizeof(a->password)-1);
    a->status = 1; a->unlock_ts = 0; a->fail_count = 0;
    save_accounts();
    printf("Password reset for %s.\n", username);
}

// 8. Admin menu
void do_admin_actions() {
    if (!current_user) { printf("You must sign in first.\n"); return; }
    if (strcmp(current_user->role, "admin")!=0) { printf("You are not admin.\n"); return; }
    while(1) {
        printf("\n--- Admin Menu ---\n");
        printf("1. View all accounts\n");
        printf("2. Delete an account\n");
        printf("3. Reset password for an account\n");
        printf("4. Back\n");
        printf("Your choice: ");
        char choice[8];
        if (!fgets(choice, sizeof(choice), stdin)) return;
        int c = atoi(choice);
        if (c==1) admin_list_accounts();
        else if (c==2) admin_delete_account();
        else if (c==3) admin_reset_other_password();
        else break;
    }
}

// 9. Sign out
void do_sign_out() {
    if (!current_user) { printf("No user is signed in.\n"); return; }
    printf("User %s signed out.\n", current_user->username);
    current_user = NULL;
}

// Dam bao ton tai tai khoan admin, neu khong thi tao tai khoan admin mac dinh
void ensure_admin_exists() {
    Account *p = head;
    while(p) {
        if (strcmp(p->role, "admin")==0) return;
        p = p->next;
    }
    // Neu khong ton tai thi tao tai khoan admin mac dinh
    Account *a = create_account_node();
    strcpy(a->username, "admin");
    strcpy(a->password, "admin");
    strcpy(a->email, "admin@local");
    strcpy(a->phone, "000");
    a->status = 1;
    strcpy(a->role, "admin");
    a->unlock_ts = 0;
    push_account(a);
    save_accounts();
    printf("No admin found. Default admin created: username='admin' password='admin'\n");
}

int main() {
    load_accounts();
    ensure_admin_exists();
    char cmd[16];
    while(1) {
        print_menu();
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        int choice = atoi(cmd);
        switch(choice) {
            case 1: do_register(); break;
            case 2: do_sign_in(); break;
            case 3: do_change_password(); break;
            case 4: do_update_info(); break;
            case 5: do_reset_password(); break;
            case 6: do_view_history(); break;
            case 7: do_auto_unlock_scan(); break;
            case 8: do_admin_actions(); break;
            case 9: do_sign_out(); break;
            default:
                printf("Exiting program.\n");
                save_accounts();
                free_accounts();
                return 0;
        }
    }
    save_accounts();
    free_accounts();
    return 0;
}
