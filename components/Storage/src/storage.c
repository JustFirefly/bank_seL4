#include <stdio.h>
#include <camkes.h>

// Matches the provides Disk disk; interface
int disk_save_state(int new_balance) {
    printf("[Storage] Writing updated ledger state ($%d) to secure disk...\n", new_balance);
    printf("[Storage] Write successful.\n");
    return 0;
}
