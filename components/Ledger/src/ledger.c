#include <camkes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "sqlite3.h"
#include <sys/types.h>
#include <sel4/sel4.h>

// This implementation will catch all printf calls and send them to the kernel debug console
ssize_t write(int fd, const void *data, size_t count) {
    const char *str = (const char *)data;
    for (size_t i = 0; i < count; i++) {
        seL4_DebugPutChar(str[i]);
    }
    return (ssize_t)count;
}
static sqlite3 *db = NULL;

/* =========================================================================
 * CUSTOM seL4 SQLITE VIRTUAL FILE SYSTEM (VFS)
 * ========================================================================= */

typedef struct sel4_file {
    sqlite3_file base;
} sel4_file;

/* Intercept SQLite Read -> Proxy to Storage Enclave */
static int sel4_xRead(sqlite3_file *file, void *buf, int iAmt, sqlite3_int64 iOfst) {
    char *out_buf = NULL;
    size_t out_sz = 0;

    /* FIX: Swapped &out_sz and &out_buf */
    int rc = disk_read((int)iOfst, iAmt, &out_sz, &out_buf);
    if (rc != 0 || out_buf == NULL) return SQLITE_IOERR_READ;

    memcpy(buf, out_buf, iAmt);
    free(out_buf); /* Free CAmkES allocated memory */
    return SQLITE_OK;
}

/* Intercept SQLite Write -> Proxy to Storage Enclave */
static int sel4_xWrite(sqlite3_file *file, const void *buf, int iAmt, sqlite3_int64 iOfst) {
    /* FIX: Swapped iAmt (size) and buf */
    int rc = disk_write((int)iOfst, iAmt, iAmt, (const char *)buf);
    return (rc == 0) ? SQLITE_OK : SQLITE_IOERR_WRITE;
}

/* Intercept File Size Check */
static int sel4_xFileSize(sqlite3_file *file, sqlite3_int64 *pSize) {
    int size = 0;
    if (disk_get_size(&size) == 0) {
        *pSize = size;
        return SQLITE_OK;
    }
    return SQLITE_IOERR;
}

/* FIX: Added missing Truncate intercept */
static int sel4_xTruncate(sqlite3_file *file, sqlite3_int64 size) {
    /* For a basic block device, we can just stub this to OK */
    return SQLITE_OK;
}

/* Sync and Close intercepts */
static int sel4_xSync(sqlite3_file *file, int flags) { disk_sync(); return SQLITE_OK; }
static int sel4_xClose(sqlite3_file *file) { disk_close(); return SQLITE_OK; }

/* Stubbing locking mechanisms */
static int sel4_xLock(sqlite3_file *file, int lockType) { return SQLITE_OK; }
static int sel4_xUnlock(sqlite3_file *file, int lockType) { return SQLITE_OK; }
static int sel4_xCheckReservedLock(sqlite3_file *file, int *pResOut) { *pResOut = 0; return SQLITE_OK; }
static int sel4_xFileControl(sqlite3_file *file, int op, void *pArg) { return SQLITE_NOTFOUND; }
static int sel4_xSectorSize(sqlite3_file *file) { return 4096; }
static int sel4_xDeviceCharacteristics(sqlite3_file *file) { return 0; }

/* FIX: Placed sel4_xTruncate in the correct struct position */
static const sqlite3_io_methods sel4_io_methods = {
    1, sel4_xClose, sel4_xRead, sel4_xWrite, sel4_xTruncate,
    sel4_xSync, sel4_xFileSize, sel4_xLock, sel4_xUnlock,
    sel4_xCheckReservedLock, sel4_xFileControl, sel4_xSectorSize,
    sel4_xDeviceCharacteristics
};

/* VFS Open Method */
static int sel4_xOpen(sqlite3_vfs *vfs, const char *zName, sqlite3_file *file, int flags, int *pOutFlags) {
    disk_open(zName ? zName : "main_db");
    file->pMethods = &sel4_io_methods;
    if (pOutFlags) *pOutFlags = flags;
    return SQLITE_OK;
}

/* Stubbing OS-level VFS features we don't need on seL4 */
static int sel4_xDelete(sqlite3_vfs *vfs, const char *zName, int syncDir) { return SQLITE_OK; }
static int sel4_xAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut) { *pResOut = 1; return SQLITE_OK; }
static int sel4_xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut) {
    sqlite3_snprintf(nOut, zOut, "%s", zName); return SQLITE_OK;
}

/* The VFS mapping struct */
static sqlite3_vfs sel4_vfs = {
    1, sizeof(sel4_file), 32, NULL, "sel4_vfs", NULL,
    sel4_xOpen, sel4_xDelete, sel4_xAccess, sel4_xFullPathname,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* =========================================================================
 * SQLITE CALLBACKS
 * ========================================================================= */

/**
 * log_viewer_callback: Executed by SQLite for each row returned by a SELECT query.
 * * @param data      Pointer to context data provided in the 4th arg of sqlite3_exec (unused here)
 * @param argc      The number of columns in the result
 * @param argv      An array of pointers to strings representing the column data
 * @param azColName An array of pointers to strings representing the column names
 * @return 0 to tell SQLite to continue processing the next row, non-zero to abort.
 */
static int log_viewer_callback(void *data, int argc, char **argv, char **azColName) {
    /* Cast unused parameter to void to prevent compiler warnings */
    (void)data;

    printf("[Ledger] Transaction Log Entry -> ");

    for (int i = 0; i < argc; i++) {
        /* Safely print the column name and value, defaulting to "NULL" if empty */
        printf("%s: %s", azColName[i], argv[i] ? argv[i] : "NULL");

        /* Add a separator between columns, but not after the last one */
        if (i < argc - 1) {
            printf(" | ");
        }
    }
    printf("\n");

    return 0;
}

/* =========================================================================
 * INITIALIZATION & BANKING LOGIC
 * ========================================================================= */

void pre_init(void) {
    printf("[Ledger] Registering secure seL4 VFS...\n");

    /* 1. Register our custom VFS as the default */
    sqlite3_vfs_register(&sel4_vfs, 1);

    /* 2. Open the database */
    int rc = sqlite3_open_v2("bank_ledger.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, "sel4_vfs");

    if (rc != SQLITE_OK) {
        printf("[Ledger] FATAL: Cannot open database. Aborting!\n");
        // Force the component to stop or enter an infinite loop to prevent
        // calling bank_execute_transaction with a NULL db.
        while(1);
    }

    /* 3. Create schema using a clearly defined string */
    char *err_msg = NULL;
    char *sql = "CREATE TABLE IF NOT EXISTS accounts (username TEXT PRIMARY KEY, balance INTEGER);"
    "CREATE TABLE IF NOT EXISTS transactions (id INTEGER PRIMARY KEY AUTOINCREMENT, type TEXT, src TEXT, dest TEXT, amount INTEGER, status TEXT);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("[Ledger] SQL Error during schema creation: %s\n", err_msg);
        sqlite3_free(err_msg);
        return;
    }

    printf("[Ledger] SQLite DB mounted over IPC successfully.\n");
}

int bank_execute_transaction(int token, const char *src, const char *dest, const char *tx_type, int amount) {
    // SAFETY GUARD
    if (db == NULL) {
        printf("[Ledger] ERR: Cannot execute transaction. Database is not initialized.\n");
        return -1;
    }

    char sql[512];
    char *err_msg = 0;
    
    // Check against dynamically issued tokens (> 1000)
    if (token < 1000) {
        printf("[Ledger] SECURITY ALERT: Unauthorized Token. Intercepting action.\n");
        sprintf(sql, "INSERT INTO transactions (src, dest, type, amount, status) VALUES ('%s', '%s', '%s', %d, 'BLOCKED_INVALID_CAPABILITY');", src, dest, tx_type, amount);
        sqlite3_exec(db, sql, 0, 0, 0);
        goto dump_live_contents;
    }
    
    // JIT Account Creation: If an account doesn't exist, create it with a $0 balance dynamically
    sprintf(sql, 
        "INSERT OR IGNORE INTO accounts (user, balance) VALUES ('%s', 0);"
        "INSERT OR IGNORE INTO accounts (user, balance) VALUES ('%s', 0);", 
        src, dest);
    sqlite3_exec(db, sql, 0, 0, 0);
    
    // Execute Dynamic Business Logic
    if (strcmp(tx_type, "TRANSFER") == 0) {
        sprintf(sql, 
            "UPDATE accounts SET balance = balance - %d WHERE user = '%s';"
            "UPDATE accounts SET balance = balance + %d WHERE user = '%s';", 
            amount, src, amount, dest);
    } else if (strcmp(tx_type, "DEPOSIT") == 0) {
        sprintf(sql, "UPDATE accounts SET balance = balance + %d WHERE user = '%s';", amount, dest);
    } else if (strcmp(tx_type, "WITHDRAWAL") == 0) {
        sprintf(sql, "UPDATE accounts SET balance = balance - %d WHERE user = '%s';", amount, src);
    }
    
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    
    sprintf(sql, "INSERT INTO transactions (src, dest, type, amount, status) VALUES ('%s', '%s', '%s', %d, 'APPROVED');", src, dest, tx_type, amount);
    sqlite3_exec(db, sql, 0, 0, 0);

dump_live_contents:
    printf("\n[SQLite Engine] >>> DUMPING LATEST LOG AUDIT RECORD <<<\n");
    sqlite3_exec(db, "SELECT id, type, src, dest, amount, status FROM transactions ORDER BY id DESC LIMIT 1;", log_viewer_callback, 0, 0);
    printf("[SQLite Engine] >>> DUMPING CURRENT STATE ACCOUNT MATRICES <<<\n");
    sqlite3_exec(db, "SELECT user, balance FROM accounts;", log_viewer_callback, 0, 0);
    printf("=================================================================\n");

    return (token >= 1000) ? 0 : -1;
}
