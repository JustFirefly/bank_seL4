#include <camkes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Matches 'int send(in string buffer)'
int socket_send(const char *buffer) {
    printf("NetworkStack: Sending data: %s\n", buffer);
    // Add your actual networking code here
    return 0;
}

// Matches 'int receive(out string buffer)'
// CAmkES requires char** for out strings
int socket_receive(char **buffer) {
    printf("NetworkStack: Waiting to receive...\n");

    // Create a dummy string to simulate receiving data
    char *msg = "Sample Data";
    *buffer = strdup(msg);

    return strlen(msg);
}

int run(void) {
    printf("NetworkStack: Running...\n");
    while(1) {
        // Main loop
    }
    return 0;
}
