cmake_minimum_required(VERSION 3.10)

if(POLICY CMP0079)
    cmake_policy(SET CMP0079 NEW)
endif()

project(banking_system C)


DeclareCAmkESComponent(Client SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(AuthServer SOURCES components/AuthServer/src/auth.c)
DeclareCAmkESComponent(Storage SOURCES components/Storage/src/storage.c)

DeclareCAmkESComponent(Ledger SOURCES 
    components/Ledger/src/ledger.c 
    components/Ledger/src/sqlite3.c
)

set_property(SOURCE components/Ledger/src/sqlite3.c APPEND PROPERTY COMPILE_OPTIONS
    "-DSQLITE_OMIT_LOAD_EXTENSION"
    "-DSQLITE_THREADSAFE=0"
    "-DSQLITE_OMIT_WAL"
)

# 1. Add the upstream lwIP project to the CMake build tree
add_subdirectory("${CMAKE_SOURCE_DIR}/../lwip" lwip_build EXCLUDE_FROM_ALL)

# 2. Tell the lwIP core target where to find both your custom headers AND its own headers
target_include_directories(lwipcore PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/components/NetworkStack/include"
    "${CMAKE_SOURCE_DIR}/../lwip/src/include"
)

# 3. NEW: Tell lwIP to use the seL4 C library so it can find <stdint.h> and other standard headers
target_link_libraries(lwipcore PRIVATE muslc)

# 4. Declare your component
DeclareCAmkESComponent(NetworkStack
    SOURCES
        components/NetworkStack/src/network_stack.c
    LIBS
        sel4
        platsupport
        lwipcore
        virtio
    INCLUDES
        components/NetworkStack/include
        "${CMAKE_SOURCE_DIR}/../lwip/src/include"
)

DeclareCAmkESComponent(APIGateway
    SOURCES components/APIGateway/src/gateway.c
)



# 4. Set the assembly blueprint as the root of the application
DeclareCAmkESRootserver(banking_system.camkes)
