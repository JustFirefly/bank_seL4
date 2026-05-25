#include <camkes.h>
#include <stdio.h>
#include <string.h>

/* Simulate a 1MB Physical Block Device */
#define DISK_SIZE (1024 * 1024)
static char physical_disk[DISK_SIZE];
static int current_file_size = 0;

int disk_open(const char *filename) {
    printf("[Storage] Opening secure database volume: %s\n", filename);
    return 0; // Success
}

int disk_close(void) {
    return 0;
}

int disk_read(int offset, int amount, size_t *buf_sz, char **buf) {
    *buf_sz = 0;
    *buf = NULL;

    // 1. If we try to read past end-of-file, simply return 0 bytes successfully.
    // SQLite will interpret this as an empty file/new database.
    if (offset >= current_file_size) {
        *buf = calloc(amount, 1); // Return zero-filled buffer
        *buf_sz = amount;
        return 0; // Success
    }

    // 2. If the request goes partially past EOF, clamp it to what's available
    int bytes_to_read = amount;
    if (offset + amount > current_file_size) {
        bytes_to_read = current_file_size - offset;
    }

    *buf = malloc(amount);
    if (!*buf) return -1;

    memset(*buf, 0, amount); // Ensure buffer is clean
    memcpy(*buf, physical_disk + offset, bytes_to_read);

    *buf_sz = amount;
    return 0; // Success
}

int disk_write(int offset, int amount, size_t buf_sz, const char *buf) {
    if (offset + amount > DISK_SIZE) {
        printf("[Storage] ERR: Out of disk space!\n");
        return -1;
    }

    // Use buf_sz instead of amount just to be safe
    memcpy(physical_disk + offset, buf, buf_sz);

    if (offset + amount > current_file_size) {
        current_file_size = offset + amount;
    }
    return 0;
}

int disk_get_size(int *size) {
    *size = current_file_size;
    return 0;
}

int disk_sync(void) {
    /* In production: Issue FLUSH command to hardware controller */
    printf("[Storage] Disk synchronized to hardware.\n");
    return 0;
}
