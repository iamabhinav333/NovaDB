cmake_minimum_required(VERSION 3.20)
project(NovaDB VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NOVADB_BUILD_TESTS "Build NovaDB tests" ON)

add_library(novadb STATIC
    src/logger.cpp
    src/config.cpp
    src/disk_manager.cpp
    src/page.cpp
    src/page_manager.cpp
    src/record.cpp
    src/record_page.cpp
    src/catalog.cpp
    src/table_manager.cpp
    src/buffer_pool.cpp
    src/bplustree.cpp
    src/query_engine.cpp
    src/transaction.cpp
    src/log_record.cpp
    src/write_ahead_log.cpp
    src/lock_manager.cpp
    src/query_optimizer.cpp
)

target_include_directories(novadb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(novadb
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(novadb PRIVATE /W4 /permissive-)
else()
    target_compile_options(novadb PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(novadb_demo src/main.cpp)
target_link_libraries(novadb_demo PRIVATE novadb)

if(NOVADB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(benchmarks)
