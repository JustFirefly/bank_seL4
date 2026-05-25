#include <camkes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sel4/sel4.h>

/* REPLACE the existing read_input function with this mock version */
void read_input(char *buffer, int max_len) {
    // Format: tx <token> <src> <dest> <type> <amount>
    // Use token 1001 to pass the security check
    const char* mock_data = "tx 1001 Alice Bob TRANSFER 100";
    strncpy(buffer, mock_data, max_len - 1);
    buffer[max_len - 1] = '\0';

    // Simulate a slight delay if needed
    for (int i = 0; i < 100000; i++) { seL4_Yield(); }
}

int run(void) {
    printf("[Client] Shared Memory Bridge Online.\n");
    char input_buffer[512];

    while (1) {
        /* 1. Get user input */
        read_input(input_buffer, sizeof(input_buffer));
        printf("[Client] Writing to shared memory: '%s'\n", input_buffer);

        /* 2. Write data to the Shared Dataport (net_buffer)
         * 'net_buffer' is automatically provided by CAmkES
         * based on your Client.camkes definition. */
        memcpy(net_buffer, input_buffer, strlen(input_buffer) + 1);

        /* 3. Signal the Gateway that data is ready
         * This uses the 'emits Notification data_ready' interface */
        data_ready_emit();

        /* 4. Wait for Gateway to signal back (Optional: depends on your flow)
         * We assume the Gateway puts the result back in the same buffer
         * or a separate reply buffer */

        printf("[Client] Waiting for response...\n");
        // We poll briefly or wait for a notification if implemented
        for (int i = 0; i < 100000; i++) { seL4_Yield(); }

        /* 5. Read result from Shared Memory */
        printf("[Client] Response: '%s'\n", (char *)net_buffer);
    }
    return 0;
}
