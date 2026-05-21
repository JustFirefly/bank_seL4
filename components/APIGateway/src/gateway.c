#include <camkes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Note: Function names are derived from instance name 'net' and IDL method names 'receive'/'send'
// This matches your definition: 'uses Network net;'

int run(void) {
    printf("APIGateway: Service started and waiting for requests.\n");

    char *buffer = NULL;

    while(1) {
        // 1. Blocking wait for incoming network data
        int bytes = net_receive(&buffer);

        if (bytes > 0) {
            printf("APIGateway: Received %d bytes\n", bytes);

            // 2. Simple command parsing logic
            if (strncmp(buffer, "LOGIN", 5) == 0) {
                // Example: Call AuthServer (instance 'auth' in APIGateway)
                // int token = auth_login("user", "pass");
                net_send("OK: Authenticated");
            }
            else if (strncmp(buffer, "TRANSFER", 8) == 0) {
                // Example: Call Ledger (instance 'bank' in APIGateway)
                // int result = bank_execute_transaction(...);
                net_send("OK: Transaction Processed");
            }
            else {
                net_send("ERR: Invalid Command");
            }

            // 3. IMPORTANT: Free memory allocated by the IPC marshalling
            free(buffer);
            buffer = NULL;
        }
    }
    return 0;
}
