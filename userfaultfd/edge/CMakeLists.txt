cmake_minimum_required(VERSION 3.17)
project(edge)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(edge "")

target_link_libraries(edge PRIVATE Threads::Threads)
target_include_directories(edge PRIVATE ./include)

target_sources(edge PRIVATE
        src/main.cpp
        src/mapped_buf.cpp
        src/fault_handler_memory.cpp
        src/interop.cpp
        src/fault_registration.cpp
        src/fault_msg.cpp
        src/internal/uffd.cpp
        )

