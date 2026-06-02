cmake_minimum_required(VERSION 3.20)
project(seminar_cpp LANGUAGES CXX)

option(SEMINAR_BUILD_CLI_TARGETS "Build standalone C++ benchmark executables" ON)
option(SEMINAR_BUILD_GOOGLE_BENCHMARKS "Build Google Benchmark suites" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP QUIET)
find_package(benchmark CONFIG QUIET)

file(GLOB TASKFLOW_HINTS
    "/opt/homebrew/Cellar/taskflow/*/include"
    "/usr/local/Cellar/taskflow/*/include")
find_path(TASKFLOW_INCLUDE_DIR taskflow/taskflow.hpp
    HINTS ${TASKFLOW_HINTS} /opt/homebrew/include /usr/local/include)

add_library(seminar_cpp_config INTERFACE)
target_include_directories(seminar_cpp_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(TASKFLOW_INCLUDE_DIR)
    target_include_directories(seminar_cpp_config INTERFACE ${TASKFLOW_INCLUDE_DIR})
endif()
if(OpenMP_CXX_FOUND)
    target_link_libraries(seminar_cpp_config INTERFACE OpenMP::OpenMP_CXX)
endif()

function(add_seminar_cpp_target target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE seminar_cpp_config)
endfunction()

if(SEMINAR_BUILD_CLI_TARGETS)
    if(OpenMP_CXX_FOUND)
        add_seminar_cpp_target(histogram_openmp histogram/openmp.cpp)
        add_seminar_cpp_target(mergesort_openmp mergesort/openmp.cpp)
        add_seminar_cpp_target(stencil_openmp stencil/openmp.cpp)
        add_seminar_cpp_target(bfs_openmp bfs/openmp.cpp)
    else()
        message(STATUS "OpenMP not found; skipping OpenMP CLI targets")
    endif()

    if(TASKFLOW_INCLUDE_DIR)
        add_seminar_cpp_target(histogram_taskflow histogram/taskflow.cpp)
        add_seminar_cpp_target(mergesort_taskflow mergesort/taskflow.cpp)
        add_seminar_cpp_target(stencil_taskflow stencil/taskflow.cpp)
        add_seminar_cpp_target(bfs_taskflow bfs/taskflow.cpp)
    else()
        message(STATUS "Taskflow headers not found; skipping Taskflow CLI targets")
    endif()
endif()

if(SEMINAR_BUILD_GOOGLE_BENCHMARKS)
    if(benchmark_FOUND)
        add_executable(seminar_google_benchmarks benchmarks/google_benchmarks.cpp)
        target_link_libraries(seminar_google_benchmarks PRIVATE seminar_cpp_config benchmark::benchmark)
    else()
        message(STATUS "Google Benchmark not found; skipping seminar_google_benchmarks target")
    endif()
endif()
