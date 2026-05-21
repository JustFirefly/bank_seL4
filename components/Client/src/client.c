#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <camkes.h>

void spin_delay() {
    for (volatile int i = 0; i < 200000000; i++) {} // ~1 second delay
}

int run(void) {
    printf("\n=========================================\n");
    printf(" seL4 Continuous Network Server (Mocked) \n");
    printf("=========================================\n\n");

    // Pre-register an automated "System Node" to act as the main fund distributor
    auth_register_user("system_node", "admin_key");
    int sys_token = auth_login("system_node", "admin_key");
    
    // Seed the system node with initial capital
    bank_execute_transaction(sys_token, "external_bank", "system_node", "DEPOSIT", 50000);

    int packet_count = 0;

    // Infinite loop simulating a live stream of network requests
    while(1) {
        spin_delay();
        packet_count++;
        
        // Randomly generate an incoming network payload
        int cmd = rand() % 100;
        char remote_ip[16];
        sprintf(remote_ip, "192.168.1.%d", (rand() % 50) + 10); // Mock IP address
        
        char dynamic_user[32];
        sprintf(dynamic_user, "user_%d", rand() % 6); // Dynamically handles users 0 through 5

        printf("\n[eth0] RX Packet #%d from %s\n", packet_count, remote_ip);

        if (cmd < 20) {
            // 20% chance the payload is a new user registration API call
            printf(" >> Payload: API_REGISTER %s\n", dynamic_user);
            auth_register_user(dynamic_user, "0000");
        } 
        else if (cmd < 40) {
            // 20% chance an external deposit hits the system
            int amount = (rand() % 500) + 50;
            printf(" >> Payload: API_DEPOSIT $%d to %s\n", amount, dynamic_user);
            bank_execute_transaction(sys_token, "external_bank", dynamic_user, "DEPOSIT", amount);
        }
        else {
            // 60% chance the system node routes money to active users
            int amount = (rand() % 100) + 1;
            printf(" >> Payload: API_TRANSFER system_node -> %s ($%d)\n", dynamic_user, amount);
            bank_execute_transaction(sys_token, "system_node", dynamic_user, "TRANSFER", amount);
        }
        printf("-----------------------------------------\n");
    }
    return 0;
}
