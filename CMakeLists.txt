cmake_minimum_required(VERSION 3.8)
project(socket_demo)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(client src/client.cpp)
add_executable(server src/server.cpp)

# target_include_directories(agent PRIVATE libs/boost_1_71_0/include)

# target_link_libraries(agent PRIVATE pthread rt)
