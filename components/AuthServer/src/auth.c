#include <stdio.h>
#include <string.h>
#include <camkes.h>

#define MAX_USERS 50

struct UserRecord {
    char user[32];
    char pin[32];
};

static struct UserRecord users[MAX_USERS];
static int num_users = 0;
static int next_token_id = 1000;

int auth_register_user(const char *user, const char *pin) {
    if (num_users >= MAX_USERS) {
        printf("[AuthServer] Error: User database full.\n");
        return -1;
    }
    
    // Save the new user into the isolated memory array
    strcpy(users[num_users].user, user);
    strcpy(users[num_users].pin, pin);
    num_users++;
    
    printf("[AuthServer] Successfully registered new user: %s\n", user);
    return 0;
}

int auth_login(const char *user, const char *pin) {
    printf("[AuthServer] Received login request for user: %s\n", user);
    
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].user, user) == 0 && strcmp(users[i].pin, pin) == 0) {
            next_token_id++; // Generate a new dynamic "capability token"
            printf("[AuthServer] Credentials valid. Issuing dynamic token: %d\n", next_token_id);
            return next_token_id;
        }
    }
    
    printf("[AuthServer] Invalid credentials.\n");
    return -1;
}
