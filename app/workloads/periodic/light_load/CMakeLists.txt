# Build configuration for Light Load workload

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(workload_light_load)

# Add common headers directory (go up to workloads/ then to common/)
target_include_directories(app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# Add source files
target_sources(app PRIVATE src/main.c)
