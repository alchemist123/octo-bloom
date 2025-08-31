cmake_minimum_required(VERSION 3.12)
project(octo-bloom LANGUAGES C CXX)

# Find PostgreSQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(PostgreSQL REQUIRED libpq)

# Get PostgreSQL configuration
execute_process(COMMAND pg_config --includedir-server OUTPUT_VARIABLE PG_INCLUDEDIR_SERVER OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pg_config --pkglibdir OUTPUT_VARIABLE PG_PKGLIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND pg_config --sharedir OUTPUT_VARIABLE PG_SHAREDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# Include directories
include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories(${PG_INCLUDEDIR_SERVER})

# Find OpenSSL for SHA256
find_package(OpenSSL REQUIRED)

# Set compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -fPIC -fno-exceptions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")

# Add source files
set(SOURCES
    src/octo_bloom.cpp
    src/bloom_filter.cpp
    src/shared_memory.cpp
    src/trigger_manager.cpp
    src/background_worker.cpp
)

# Create extension
add_library(octo_bloom MODULE ${SOURCES})

# Link PostgreSQL libraries
target_link_libraries(octo_bloom
    ${PostgreSQL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Set output directory
set_target_properties(octo_bloom PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    PREFIX ""
)

# Installation directives
install(TARGETS octo_bloom 
        DESTINATION ${PG_PKGLIBDIR})

install(FILES sql/octo_bloom--1.0.sql 
        DESTINATION ${PG_SHAREDIR}/extension)

install(FILES octo_bloom.control 
        DESTINATION ${PG_SHAREDIR}/extension)