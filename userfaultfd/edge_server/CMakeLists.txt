cmake_minimum_required(VERSION 3.17)
project(edge_server)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(edge_server "")

target_link_libraries(edge_server PRIVATE edge Threads::Threads)
target_include_directories(edge_server PRIVATE ../include)

target_sources(edge_server PRIVATE
        src/main.cpp
        )

