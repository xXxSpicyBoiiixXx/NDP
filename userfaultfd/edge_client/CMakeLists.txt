cmake_minimum_required(VERSION 3.17)
project(edge_client)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(edge_client "")

target_link_libraries(edge_client PRIVATE edge Threads::Threads)
target_include_directories(edge_client PRIVATE ../include)

target_sources(edge_client PRIVATE
        src/main.cpp
        )

