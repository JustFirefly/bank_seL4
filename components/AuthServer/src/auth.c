#include <stdio.h>
#include <string.h>
#include <camkes.h>

// Matches the provides Auth auth; interface
int auth_login(const char *user, const char *pin) {
    printf("[AuthServer] Received login request for user: %s\n", user);
    
    if (strcmp(user, "alice") == 0 && strcmp(pin, "1234") == 0) {
        printf("[AuthServer] Credentials valid. Issuing token.\n");
        return 999; // Mock secure token
    }
    
    printf("[AuthServer] Invalid credentials.\n");
    return -1;
}
