cmake_minimum_required(VERSION 3.10)

project(banking_system C)

# Declare each component and point to its C source code
DeclareCAmkESComponent(Client SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(AuthServer SOURCES components/AuthServer/src/auth.c)
DeclareCAmkESComponent(Ledger SOURCES components/Ledger/src/ledger.c)
DeclareCAmkESComponent(Storage SOURCES components/Storage/src/storage.c)

# Set the assembly blueprint as the root of the application
DeclareCAmkESRootserver(banking_system.camkes)
