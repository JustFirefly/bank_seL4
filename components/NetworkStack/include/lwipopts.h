/* components/NetworkStack/include/lwipopts.h */
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

/* NO_SYS = 1 tells lwIP we are running in a single-threaded bare-metal
 * polling loop without a separate OS-level TCP/IP background thread. */
#define NO_SYS                      1

/* CRITICAL FIX: Disable OS-dependent APIs that require threading/mailboxes */
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0
#define LWIP_NETIF_API 0

/* CRITICAL FIX: Disable interrupt memory protection since our enclave
 * is strictly single-threaded and cooperatively polled. */
#define SYS_LIGHTWEIGHT_PROT        0

/* Enable IPv4 and TCP */
#define LWIP_IPV4                   1
#define LWIP_TCP                    1

/* Disable protocols we don't need right now to speed up compilation */
#define LWIP_UDP                    0
#define LWIP_DHCP                   0
#define LWIP_IGMP                   0

/* Basic Memory Settings (Allocate a 2MB heap for network packets) */
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (2 * 1024 * 1024)

/* Enable the raw callback API which we used in network_stack.c */
#define LWIP_CALLBACK_API           1

#endif /* LWIPOPTS_H */
