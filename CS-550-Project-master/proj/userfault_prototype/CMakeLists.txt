cmake_minimum_required(VERSION 3.17)
project(edge)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(edge "")
target_sources(edge PRIVATE src/main.cpp)
target_link_libraries(edge PRIVATE Threads::Threads)
target_include_directories(edge PRIVATE ./include)
