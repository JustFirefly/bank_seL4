#include <stdio.h>
#include <string.h>
#include <camkes.h>

// A simple structure to simulate incoming API requests
struct Request {
    const char *command;
    const char *arg1;
    const char *arg2;
};

// Artificial delay so the terminal doesn't instantly flash by
void spin_delay() {
    for (volatile int i = 0; i < 90000000; i++) {}
}

int run(void) {
    printf("\n===================================\n");
    printf("   seL4 Automated Batch Processor\n");
    printf("===================================\n\n");

    // Our simulated incoming network queue
    struct Request queue[] = {
        {"transfer", "bob", "50"},        // Will fail: Not logged in
        {"login", "hacker", "0000"},      // Will fail: Bad credentials
        {"login", "alice", "1234"},       // Will succeed: Issues token
        {"transfer", "bob", "50"},        // Will succeed: Valid token
        {"transfer", "eve", "9000"},      // Will fail: Insufficient funds
        {"END", "", ""}
    };

    int current_token = -1;
    int i = 0;

    while (strcmp(queue[i].command, "END") != 0) {
        spin_delay();
        
        printf("seL4-Bank> %s %s %s\n", queue[i].command, queue[i].arg1, queue[i].arg2);
        
        if (strcmp(queue[i].command, "login") == 0) {
            printf("[Client] IPC Call -> AuthServer...\n");
            current_token = auth_login(queue[i].arg1, queue[i].arg2);
            
            if (current_token != -1) {
                printf("[Client] Login success! Session token: %d\n", current_token);
            } else {
                printf("[Client] Login failed. Invalid credentials.\n");
            }
        } 
        else if (strcmp(queue[i].command, "transfer") == 0) {
            if (current_token == -1) {
                printf("[Client] Error: You must login first.\n");
            } else {
                int amount;
                sscanf(queue[i].arg2, "%d", &amount);
                
                printf("[Client] IPC Call -> Ledger...\n");
                int status = bank_transfer(current_token, queue[i].arg1, amount);
                
                if (status == 0) {
                    printf("[Client] Transfer of $%d to %s complete!\n", amount, queue[i].arg1);
                } else {
                    printf("[Client] Transfer denied by Ledger.\n");
                }
            }
        }
        
        printf("-----------------------------------\n");
        i++;
    }

    printf("\n[Client] Request queue empty. Shutting down.\n");
    return 0;
}
