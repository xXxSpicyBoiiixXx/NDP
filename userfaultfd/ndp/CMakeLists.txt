cmake_minimum_required(VERSION 3.17)
project(ndp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_C_STANDARD 11)

add_executable(ndp "")
target_link_libraries(ndp PRIVATE Threads::Threads)
target_sources(ndp PRIVATE
        src/main.c
        src/mapped_buf.c
        )

target_include_directories(ndp PRIVATE ./include)
