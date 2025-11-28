cmake_minimum_required(VERSION 3.10)
project(AlgoVault)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable more warnings (recommended for C++ projects)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external
)

# Source files
set(SOURCES
    src/kvstore.cpp
    src/lru_cache.cpp
    src/persistence.cpp
    src/server.cpp
    main.cpp
)

add_executable(algovault ${SOURCES})
