#ifndef VIRTIO_MMIO_H
#define VIRTIO_MMIO_H

#include <stdint.h>

typedef struct {
    volatile uint32_t MagicValue;       // Offset 0x000
    volatile uint32_t Version;          // Offset 0x004
    volatile uint32_t DeviceID;         // Offset 0x008
    volatile uint32_t VendorID;         // Offset 0x00c
    volatile uint32_t DeviceFeatures;   // Offset 0x010
    volatile uint32_t DeviceFeaturesSel; // Offset 0x014
    uint32_t _reserved1[2];
    volatile uint32_t DriverFeatures;   // Offset 0x020
    volatile uint32_t DriverFeaturesSel; // Offset 0x024
    uint32_t _reserved2[2];
    volatile uint32_t QueueSel;         // Offset 0x030
    volatile uint32_t QueueNumMax;      // Offset 0x034
    volatile uint32_t QueueNum;         // Offset 0x038
    uint32_t _reserved3[3];
    volatile uint32_t QueueReady;       // Offset 0x044
    uint32_t _reserved4[2];
    volatile uint32_t QueueNotify;      // Offset 0x050
    uint32_t _reserved5[3];
    volatile uint32_t InterruptStatus;  // Offset 0x060
    volatile uint32_t InterruptACK;     // Offset 0x064
    uint32_t _reserved6[2];
    volatile uint32_t Status;           // Offset 0x070
    uint32_t _reserved7[3];
    volatile uint32_t QueueDescLow;     // Offset 0x080
    volatile uint32_t QueueDescHigh;    // Offset 0x084
    uint32_t _reserved8[2];
    volatile uint32_t QueueAvailLow;    // Offset 0x090
    volatile uint32_t QueueAvailHigh;   // Offset 0x094
    uint32_t _reserved9[2];
    volatile uint32_t QueueUsedLow;     // Offset 0x0A0
    volatile uint32_t QueueUsedHigh;    // Offset 0x0A4
} virtio_mmio_regs_t;

#define VIRTIO_MAGIC_VALUE 0x74726976
#endif
