#include <stdio.h>
#include <string.h>
#include <camkes.h>
#include "sqlite3.h"

static sqlite3 *db;

static int log_viewer_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("  %s: %s |", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

void pre_init(void) {
    printf("[Ledger] Booting High-Assurance SQL Engine...\n");
    sqlite3_open(":memory:", &db);
    
    // No hardcoded users anymore! Just an empty dynamic schema.
    const char *sql_init = 
        "CREATE TABLE accounts(user TEXT PRIMARY KEY, balance INT);"
        "CREATE TABLE transactions("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  src TEXT,"
        "  dest TEXT,"
        "  type TEXT,"
        "  amount INT,"
        "  status TEXT"
        ");";
        
    sqlite3_exec(db, sql_init, 0, 0, 0);
    printf("[Ledger] Empty Universal Audit Database initialized successfully.\n");
}

int bank_execute_transaction(int token, const char *src, const char *dest, const char *tx_type, int amount) {
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
    disk_save_state(1);

dump_live_contents:
    printf("\n[SQLite Engine] >>> DUMPING LATEST LOG AUDIT RECORD <<<\n");
    sqlite3_exec(db, "SELECT id, type, src, dest, amount, status FROM transactions ORDER BY id DESC LIMIT 1;", log_viewer_callback, 0, 0);
    printf("[SQLite Engine] >>> DUMPING CURRENT STATE ACCOUNT MATRICES <<<\n");
    sqlite3_exec(db, "SELECT user, balance FROM accounts;", log_viewer_callback, 0, 0);
    printf("=================================================================\n");

    return (token >= 1000) ? 0 : -1;
}
