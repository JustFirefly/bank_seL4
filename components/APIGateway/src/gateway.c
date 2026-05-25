#include <camkes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int run(void) {
    printf("[Gateway] Bridge initialized. Waiting for Client data...\n");

    while (1) {
        /* Wait for the Client to signal data arrival */
        data_ready_wait();

        /* Access shared memory buffer */
        char *request = (char *)net_buffer;
        char reply[512];
        memset(reply, 0, sizeof(reply));

        /* --- PART 3: Parsing and Dispatching Logic --- */

        // We create a copy so strtok doesn't modify the shared buffer if we need it later
        char local_buf[512];
        strncpy(local_buf, request, 511);

        char *cmd = strtok(local_buf, " ");

        if (cmd == NULL) {
            snprintf(reply, sizeof(reply), "ERROR: Empty command");
        }
        else if (strcmp(cmd, "login") == 0) {
            char *user = strtok(NULL, " ");
            char *pin = strtok(NULL, " ");
            if (user && pin) {
                // Call AuthServer RPC
                int result = auth_login(user, pin);
                snprintf(reply, sizeof(reply), "LOGIN_RESULT: %d", result);
            } else {
                snprintf(reply, sizeof(reply), "ERROR: Usage: login <user> <pin>");
            }
        }
        else if (strcmp(cmd, "register") == 0) {
            char *user = strtok(NULL, " ");
            char *pin = strtok(NULL, " ");
            if (user && pin) {
                // Call AuthServer RPC
                int result = auth_register_user(user, pin);
                snprintf(reply, sizeof(reply), "REGISTER_RESULT: %d", result);
            } else {
                snprintf(reply, sizeof(reply), "ERROR: Usage: register <user> <pin>");
            }
        }
        else if (strcmp(cmd, "tx") == 0) {
            // tx <token> <src> <dest> <type> <amount>
            char *token_str = strtok(NULL, " ");
            char *src = strtok(NULL, " ");
            char *dest = strtok(NULL, " ");
            char *type = strtok(NULL, " ");
            char *amt_str = strtok(NULL, " ");

            if (token_str && src && dest && type && amt_str) {
                // Call Ledger/Bank RPC
                int result = bank_execute_transaction(atoi(token_str), src, dest, type, atoi(amt_str));
                snprintf(reply, sizeof(reply), "TX_RESULT: %d", result);
            } else {
                snprintf(reply, sizeof(reply), "ERROR: Usage: tx <token> <src> <dest> <type> <amt>");
            }
        }
        else {
            snprintf(reply, sizeof(reply), "ERROR: Unknown command: %s", cmd);
        }

        /* Copy the response back to the shared dataport for the Client to read */
        memcpy(net_buffer, reply, strlen(reply) + 1);

        printf("[Gateway] Request processed. Result: %s\n", reply);
    }
    return 0;
}
