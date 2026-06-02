cmake_minimum_required(VERSION 3.20)
project(seminar_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP QUIET)
find_package(benchmark CONFIG REQUIRED)

find_path(TASKFLOW_INCLUDE_DIR taskflow/taskflow.hpp
    HINTS
        ENV CMAKE_INCLUDE_PATH
        ENV CPLUS_INCLUDE_PATH
        ENV CPATH
        /opt/homebrew/include
        /usr/local/include
    PATH_SUFFIXES include)

add_library(seminar_cpp_config INTERFACE)
target_include_directories(seminar_cpp_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(TASKFLOW_INCLUDE_DIR)
    target_include_directories(seminar_cpp_config INTERFACE ${TASKFLOW_INCLUDE_DIR})
endif()
if(OpenMP_CXX_FOUND)
    target_link_libraries(seminar_cpp_config INTERFACE OpenMP::OpenMP_CXX)
endif()

add_executable(seminar_google_benchmarks benchmarks/google_benchmarks.cpp)
target_link_libraries(seminar_google_benchmarks PRIVATE seminar_cpp_config benchmark::benchmark)
