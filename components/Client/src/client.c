#include <camkes.h>
#include <stdio.h>
#include <string.h>

int run(void) {
    printf("Client: Started.\n");

    char *buffer = NULL; // Must be initialized to NULL for CAmkES memory management

    while(1) {
        // Correct naming: socket_receive (from interface instance 'socket' + method 'receive')
        int bytes = socket_receive(&buffer);

        if (bytes > 0) {
            printf("Client: Received %d bytes: %s\n", bytes, buffer);
            // Free the memory allocated by the NetworkStack
            free(buffer);
            buffer = NULL;
        }
    }
    return 0;
}
