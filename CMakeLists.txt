cmake_minimum_required(VERSION 3.8)
project(socket_demo)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add main library

add_library(
    socket_demo STATIC
        src/server_tcp.cpp
        src/server_udp.cpp
        src/echo_server_delegate.cpp
        src/client_tcp.cpp
        src/client_udp.cpp
)

target_include_directories(socket_demo PUBLIC include/ src/)

# Add bins

add_executable(client src/bin/client.cpp)
target_link_libraries(client PRIVATE socket_demo)

add_executable(server src/bin/server.cpp)
target_link_libraries(server PRIVATE socket_demo)

# Add tests

add_executable(smoke_test test/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE socket_demo pthread)

# Add install target

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/_stage)
install(TARGETS client server smoke_test DESTINATION ${CMAKE_INSTALL_PREFIX})
