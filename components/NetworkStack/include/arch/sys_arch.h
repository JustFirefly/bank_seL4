#include <lwip/sys.h>
#include <lwip/err.h>
#include <sel4/sel4.h>
#include <camkes.h>
#include <stdlib.h>
#include <stdio.h>

/* --- 1. Semaphores and Mutexes --- */

struct sys_sem {
    seL4_CPtr signal_cap;
};

err_t sys_sem_new(sys_sem_t *sem, u8_t count) {
    *sem = (sys_sem_t)malloc(sizeof(struct sys_sem));
    if (*sem == NULL) return ERR_MEM;
    (*sem)->signal_cap = seL4_CapNull; // Assigned by init
    return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem) {
    if (sem && (*sem)->signal_cap != seL4_CapNull) {
        seL4_Signal((*sem)->signal_cap);
    }
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
    if (!sem || (*sem)->signal_cap == seL4_CapNull) return SYS_ARCH_TIMEOUT;
    seL4_Wait((*sem)->signal_cap, NULL);
    return 0;
}

void sys_sem_free(sys_sem_t *sem) {
    if (sem) { free(*sem); *sem = NULL; }
}

/* Mutexes are semaphores initialized to count 1 */
err_t sys_mutex_new(sys_mutex_t *mutex) { return sys_sem_new(mutex, 1); }
void sys_mutex_lock(sys_mutex_t *mutex) { sys_arch_sem_wait(mutex, 0); }
void sys_mutex_unlock(sys_mutex_t *mutex) { sys_sem_signal(mutex); }
void sys_mutex_free(sys_mutex_t *mutex) { sys_sem_free(mutex); }

/* --- 2. Mailboxes --- */

#define SYS_MBOX_SIZE 32
struct sys_mbox {
    void *msgs[SYS_MBOX_SIZE];
    int head, tail, count;
    sys_sem_t sem_read;
    sys_mutex_t lock;
};

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
    *mbox = (sys_mbox_t)malloc(sizeof(struct sys_mbox));
    if (!*mbox) return ERR_MEM;
    (*mbox)->head = (*mbox)->tail = (*mbox)->count = 0;
    sys_sem_new(&(*mbox)->sem_read, 0);
    sys_mutex_new(&(*mbox)->lock);
    return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
    sys_mutex_lock(&(*mbox)->lock);
    (*mbox)->msgs[(*mbox)->head] = msg;
    (*mbox)->head = ((*mbox)->head + 1) % SYS_MBOX_SIZE;
    (*mbox)->count++;
    sys_mutex_unlock(&(*mbox)->lock);
    sys_sem_signal(&(*mbox)->sem_read);
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
    sys_arch_sem_wait(&(*mbox)->sem_read, timeout);
    sys_mutex_lock(&(*mbox)->lock);
    *msg = (*mbox)->msgs[(*mbox)->tail];
    (*mbox)->tail = ((*mbox)->tail + 1) % SYS_MBOX_SIZE;
    (*mbox)->count--;
    sys_mutex_unlock(&(*mbox)->lock);
    return 0;
}

/* --- 3. Time and Init --- */

extern uint64_t timer_get_time(void); // CAmkES provided interface

u32_t sys_now(void) {
    return (u32_t)(timer_get_time() / 1000);
}

void sys_init(void) {
    printf("lwIP sys_arch: Initialized for seL4/CAmkES.\n");
}
