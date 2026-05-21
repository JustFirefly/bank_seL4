cmake_minimum_required(VERSION 3.10)

project(banking_system C)

# 1. Declare the standard components
DeclareCAmkESComponent(Client SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(AuthServer SOURCES components/AuthServer/src/auth.c)
DeclareCAmkESComponent(Storage SOURCES components/Storage/src/storage.c)

# 2. Declare the Ledger and include the SQLite source file
DeclareCAmkESComponent(Ledger SOURCES 
    components/Ledger/src/ledger.c 
    components/Ledger/src/sqlite3.c
)

# 3. Apply strict embedded compiler flags specifically to SQLite
# This prevents it from looking for Linux OS features that seL4 lacks
set_property(SOURCE components/Ledger/src/sqlite3.c APPEND PROPERTY COMPILE_OPTIONS
    "-DSQLITE_OMIT_LOAD_EXTENSION"
    "-DSQLITE_THREADSAFE=0"
    "-DSQLITE_OMIT_WAL"
)

# 4. Set the assembly blueprint as the root of the application
DeclareCAmkESRootserver(banking_system.camkes)
