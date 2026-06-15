cmake_minimum_required(VERSION 3.20)
project(seminar_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(OpenMP REQUIRED)
find_package(benchmark CONFIG REQUIRED)

find_path(TASKFLOW_INCLUDE_DIR taskflow/taskflow.hpp REQUIRED
    HINTS
        ENV CMAKE_INCLUDE_PATH
        ENV CPLUS_INCLUDE_PATH
        ENV CPATH
        /opt/homebrew/include
        /usr/local/include
    PATH_SUFFIXES include)

add_library(seminar_cpp_config INTERFACE)
target_include_directories(seminar_cpp_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${TASKFLOW_INCLUDE_DIR})
target_link_libraries(seminar_cpp_config INTERFACE OpenMP::OpenMP_CXX)

function(add_seminar_google_benchmark target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE seminar_cpp_config benchmark::benchmark)
endfunction()

add_seminar_google_benchmark(histogram_benchmarks benchmarks/histogram.cpp)
add_seminar_google_benchmark(mergesort_benchmarks benchmarks/mergesort.cpp)
add_seminar_google_benchmark(stencil_benchmarks benchmarks/stencil.cpp)
add_seminar_google_benchmark(bfs_benchmarks benchmarks/bfs.cpp)
