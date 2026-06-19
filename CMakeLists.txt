cmake_minimum_required(VERSION 3.16)

project(vacuum
    VERSION 1.0.0
    DESCRIPTION "Organize files into per-extension subdirectories"
    LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(vacuum vacuum.c)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vacuum PRIVATE -Wall -Wextra)
endif()

include(GNUInstallDirs)
install(TARGETS vacuum RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

enable_testing()
add_test(
    NAME functional
    COMMAND ${CMAKE_COMMAND} -E env bash
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh $<TARGET_FILE:vacuum>)
