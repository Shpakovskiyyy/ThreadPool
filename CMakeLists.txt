cmake_minimum_required(VERSION 3.20)
project(ThreadPool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only interface library
add_library(ThreadPool INTERFACE)
add_library(tp::ThreadPool ALIAS ThreadPool)

target_include_directories(ThreadPool INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(ThreadPool INTERFACE cxx_std_20)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(ThreadPool INTERFACE Threads::Threads)

# Optional: example/test executable
option(TP_BUILD_EXAMPLES "Build examples" OFF)
if(TP_BUILD_EXAMPLES)
    add_executable(ThreadPoolExample examples/example.cpp)
    target_link_libraries(ThreadPoolExample PRIVATE tp::ThreadPool)
endif()