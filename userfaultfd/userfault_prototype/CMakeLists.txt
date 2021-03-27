cmake_minimum_required(VERSION 3.17)
project(uffd_prototype)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(uffd_prototype "")
target_sources(uffd_prototype PRIVATE src/main.cpp)
target_link_libraries(uffd_prototype PRIVATE Threads::Threads)
target_include_directories(uffd_prototype PRIVATE ./include)
