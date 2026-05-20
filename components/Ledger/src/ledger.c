#include <stdio.h>
#include <camkes.h>

// Ledger state
static int alice_balance = 1000;

// Matches the provides Bank bank; interface
int bank_transfer(int token, const char *to_user, int amount) {
    printf("[Ledger] Transfer request received. Validating token: %d\n", token);
    
    if (token != 999) {
        printf("[Ledger] SECURITY FAULT: Invalid token. Dropping request.\n");
        return -1;
    }
    
    if (alice_balance >= amount) {
        alice_balance -= amount;
        printf("[Ledger] Transfer approved. New balance: $%d\n", alice_balance);
        
        // Push state to Storage via IPC
        printf("[Ledger] Instructing Storage to persist state...\n");
        disk_save_state(alice_balance);
        
        return 0;
    }
    
    return -1;
}
