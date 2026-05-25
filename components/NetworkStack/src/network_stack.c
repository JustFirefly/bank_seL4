#include <camkes.h>
#include <lwip/init.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <string.h>
#include "virtio_mmio.h"
#include <virtio/virtio_ring.h>
#include <sys/types.h>
#include <sel4/sel4.h>
#include <netif/etharp.h>

// This implementation will catch all printf calls and send them to the kernel debug console
ssize_t write(int fd, const void *data, size_t count) {
    const char *str = (const char *)data;
    for (size_t i = 0; i < count; i++) {
        seL4_DebugPutChar(str[i]);
    }
    return (ssize_t)count;
}

// Add this helper function to your network_stack.c
void debug_log(const char *str) {
    while (*str) {
        seL4_DebugPutChar(*str++);
    }
}

#define RING_SIZE 256
static uint8_t rx_buffer[RING_SIZE][1536] __attribute__((aligned(4096)));
static uint8_t tx_buffer[RING_SIZE][1536] __attribute__((aligned(4096)));

static uint16_t last_rx_idx = 0;
static uint16_t tx_idx = 0;

static struct vring rx_ring;
static struct vring tx_ring;

/* Memory for the actual descriptor tables, avail rings, and used rings.
 * 8192 bytes is typically enough for a ring size of 256 with 4096 alignment. */
static uint8_t rx_ring_mem[8192] __attribute__((aligned(4096)));
static uint8_t tx_ring_mem[8192] __attribute__((aligned(4096)));

static struct netif netif;
static virtio_mmio_regs_t *regs;

/* Global flag to signal data availability */
static volatile bool data_available = false;

static err_t netif_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr) {
    // 1. Get the descriptor index from the TX ring
    uint16_t desc_idx = tx_idx % RING_SIZE;
    struct vring_desc *desc = &tx_ring.desc[desc_idx];

    // 2. Copy payload to our ring buffer
    // Note: In production, consider zero-copy if supported by your memory setup
    memcpy(tx_buffer[desc_idx], p->payload, p->len);

    // 3. Setup descriptor
    desc->addr = (uintptr_t)tx_buffer[desc_idx];
    desc->len = p->len;
    desc->flags = 0; // No flags

    // 4. Add to Available Ring
    tx_ring.avail->ring[tx_ring.avail->idx % RING_SIZE] = desc_idx;
    tx_ring.avail->idx++;

    // 5. Memory barrier: Ensure data is in memory before notification
    __sync_synchronize();

    // 6. Notify TX queue (Queue 1)
    regs->QueueNotify = 1;

    tx_idx++;
    return ERR_OK;
}

static err_t low_level_output(struct netif *netif, struct pbuf *p) {
    // This is essentially the same as your netif_output logic,
    // but without the ip4_addr argument.
    // Use your existing logic to copy p->payload to your tx_ring and notify the hardware

    // Example (refactor your tx_ring logic here):
    uint16_t desc_idx = tx_idx % RING_SIZE;
    struct vring_desc *desc = &tx_ring.desc[desc_idx];

    memcpy(tx_buffer[desc_idx], p->payload, p->len);
    desc->addr = (uintptr_t)tx_buffer[desc_idx];
    desc->len = p->len;
    desc->flags = 0;

    tx_ring.avail->ring[tx_ring.avail->idx % RING_SIZE] = desc_idx;
    tx_ring.avail->idx++;
    __sync_synchronize();
    regs->QueueNotify = 1;
    tx_idx++;

    return ERR_OK;
}

static err_t m_netif_init(struct netif *netif) {
    netif->name[0] = 'e'; netif->name[1] = 'n';
    netif->output = etharp_output;       // For IPv4/ARP
    netif->linkoutput = low_level_output; // REQUIRED: For raw Ethernet frames
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    // --- ADDED: Set a dummy locally administered MAC address ---
    netif->hwaddr_len = 6;
    netif->hwaddr[0] = 0x02; // 0x02 means locally administered
    netif->hwaddr[1] = 0x00;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x00;
    netif->hwaddr[4] = 0x00;
    netif->hwaddr[5] = 0x01;

    return ERR_OK;
}


void virtio_init_hardware(void) {
    // 1. Log the address immediately. This is safe because printing a variable
    // address does not dereference it.
    printf("[NetworkStack] Debug: virtio_mmio pointer address is %p\n", (void*)virtio_mmio);

    // 2. Simple NULL check. This is safe.
    if (virtio_mmio == NULL) {
        printf("[NetworkStack] FATAL: virtio_mmio dataport is NULL. Check CAmkES connections.\n");
        return;
    }

    // 3. Cast to the register structure
    regs = (virtio_mmio_regs_t *)virtio_mmio;

    // 4. THIS IS THE CRITICAL CHECK.
    // If it faults here, the address 0x851000 is definitely not mapped to hardware.
    printf("[NetworkStack] Debug: Attempting to access MagicValue...\n");
    uint32_t magic = regs->MagicValue;

    printf("[NetworkStack] Debug: MagicValue read: 0x%x\n", magic);

    // Check if the magic value is actually sane
    if (regs->MagicValue != 0x74726976) { // 0x74726976 is 'virt' in ASCII
        printf("[NetworkStack] Error: Invalid MagicValue at address %p. Got: 0x%x\n",
               (void*)regs, regs->MagicValue);
        return;
    }

    if (regs->MagicValue != VIRTIO_MAGIC_VALUE) {
        printf("[NetworkStack] Error: VirtIO device not found.\n");
        return;
    }

    /* Standard VirtIO initialization handshake */
    regs->Status = 0;              // Reset
    regs->Status |= 0x1;           // Acknowledge
    regs->Status |= 0x2;           // Driver
    regs->DeviceFeaturesSel = 0;
    regs->DriverFeatures = regs->DeviceFeatures;
    regs->Status |= 0x8;           // Features OK

    /* --- ADDED: Initialize the software vring structures --- */
    vring_init(&rx_ring, RING_SIZE, rx_ring_mem, 4096);
    vring_init(&tx_ring, RING_SIZE, tx_ring_mem, 4096);

    /* --- ADDED: Tell the hardware about the RX Queue (Queue 0) --- */
    regs->QueueSel = 0;
    regs->QueueNum = RING_SIZE;
    regs->QueueDescLow = (uint32_t)(uintptr_t)rx_ring.desc;
    regs->QueueDescHigh = 0;
    regs->QueueAvailLow = (uint32_t)(uintptr_t)rx_ring.avail;
    regs->QueueAvailHigh = 0;
    regs->QueueUsedLow = (uint32_t)(uintptr_t)rx_ring.used;
    regs->QueueUsedHigh = 0;
    regs->QueueReady = 1;

    /* --- ADDED: Tell the hardware about the TX Queue (Queue 1) --- */
    regs->QueueSel = 1;
    regs->QueueNum = RING_SIZE;
    regs->QueueDescLow = (uint32_t)(uintptr_t)tx_ring.desc;
    regs->QueueDescHigh = 0;
    regs->QueueAvailLow = (uint32_t)(uintptr_t)tx_ring.avail;
    regs->QueueAvailHigh = 0;
    regs->QueueUsedLow = (uint32_t)(uintptr_t)tx_ring.used;
    regs->QueueUsedHigh = 0;
    regs->QueueReady = 1;

    /* --- ADDED: Pre-fill the RX ring with empty buffers --- */
    /* Hardware needs somewhere to put incoming packets immediately */
    for (int i = 0; i < RING_SIZE; i++) {
        rx_ring.desc[i].addr = (uintptr_t)rx_buffer[i];
        rx_ring.desc[i].len = 1536;
        rx_ring.desc[i].flags = VRING_DESC_F_WRITE; // Tell hardware it can write here
        rx_ring.avail->ring[i] = i;
    }
    rx_ring.avail->idx = RING_SIZE;

    __sync_synchronize(); // Memory barrier
    regs->QueueNotify = 0; // Notify RX Queue (0) that buffers are ready

    /* Finally, set Status to DRIVER_OK (0x4) to start the device */
    regs->Status |= 0x4;

    printf("[NetworkStack] VirtIO initialized. Version: %u\n", regs->Version);
}

void hardware_irq_handle(void) {
    // CRITICAL: Prevent NULL pointer dereference if IRQ fires during or before init
    if (regs == NULL) {
        (void)hardware_irq_acknowledge();
        return;
    }

    uint32_t isr = regs->InterruptStatus;

    if (isr & 0x1) { // 0x1 is the RX interrupt bit
        uint16_t num_processed = 0;

        // Process all used descriptors
        while (last_rx_idx != rx_ring.used->idx) {
            uint16_t idx = last_rx_idx % RING_SIZE;
            struct vring_used_elem *used_elem = &rx_ring.used->ring[idx];

            uint32_t len = used_elem->len;
            uint32_t id = used_elem->id;

            struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
            if (p != NULL) {
                memcpy(p->payload, rx_buffer[id], len);
                if (netif.input(p, &netif) != ERR_OK) {
                    pbuf_free(p);
                }

                // Signal that data is ready
                data_available = true;
            }

            // Give buffer back to hardware
            rx_ring.avail->ring[rx_ring.avail->idx % RING_SIZE] = id;
            rx_ring.avail->idx++;
            num_processed++;
            last_rx_idx++;
        }

        if (num_processed > 0) {
            __sync_synchronize();
            regs->QueueNotify = 0; // Notify RX Queue
        }

        regs->InterruptACK = isr;
    }

    // REQUIRED: Tell CAmkES we are done
    (void)hardware_irq_acknowledge();
}

/* --- lwIP PORTING REQUIREMENTS --- */
// lwIP requires sys_now() to keep track of timeouts.
// For now, we will use a dummy counter. In a production seL4 app,
// you would route this to a hardware timer component.
uint32_t sys_now(void) {
    static uint32_t mock_ticks = 0;
    return mock_ticks++;
}

/* --- CAmkES RPC INTERFACE IMPLEMENTATIONS --- */
// CAmkES expects these to exist because of your Network.idl4 definitions

void net_wait_for_data(void) {
    printf("[NetworkStack] Waiting for incoming packets...\n");

    // Loop until the interrupt handler signals data is available
    while (!data_available) {
        // seL4_Yield() tells the kernel to switch to another thread
        // This prevents the CPU from pegging at 100% while waiting
        seL4_Yield();
    }

    // Reset the flag so the next call to wait_for_data blocks correctly
    data_available = false;

    printf("[NetworkStack] Packet received and data is ready.\n");
}

int net_send(const char *buffer) {
    // You can add logic here later to pass the buffer into your tx_ring
    printf("[NetworkStack] net_send called! Data: %s\n", buffer);
    return 0; // Return 0 to indicate success
}

int run(void) {
    printf("[NetworkStack] Initializing Hardware and Stack...\n");

    // 1. Initialize lwIP core first
    lwip_init();
    printf("[NetworkStack] Core initialized.\n");
    virtio_init_hardware();

    struct ip4_addr ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 10, 0, 2, 2);

    printf("[NetworkStack] netif address: %p\n", (void*)&netif);

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, m_netif_init, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);
    netif_set_link_up(&netif);

    printf("[NetworkStack] Stack active. Yielding to CAmkES event loop...\n");

    // We MUST return 0 here so the CAmkES thread can listen for interrupts
    return 0;
}
