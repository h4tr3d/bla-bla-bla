project(hotel-processing)
cmake_minimum_required(VERSION 3.14)

option(USE_CACHE "Enable Cached version" ON)

# where to look first for cmake modules, before ${CMAKE_ROOT}/modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# C++ standard: 98, 11, 14, 17, 20
#   to use per-terget: https://cmake.org/cmake/help/v3.8/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF) # turn on/off GNU or some other extensions

# C standard: 90, 99, 11
#   to use per-terget: https://cmake.org/cmake/help/v3.8/prop_tgt/C_STANDARD.html
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
#set(CMAKE_C_EXTENSIONS OFF) # turn on/off GNU or some other extensions

find_package(Threads)
#find_package(PkgConfig)
find_package(fmt REQUIRED)

# Warnings
set (WARNING_OPTIONS
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
       -Wall -Wextra>
     $<$<CXX_COMPILER_ID:MSVC>:
       /W4>)


add_library(${PROJECT_NAME}_process hotels.cpp)
add_library(${PROJECT_NAME}::process ALIAS ${PROJECT_NAME}_process)
if (USE_CACHE)
    target_compile_definitions(${PROJECT_NAME}_process PUBLIC CACHED)
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}::process)
target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_OPTIONS})

add_executable(${PROJECT_NAME}_tests tests.cpp)
target_link_libraries(${PROJECT_NAME}_tests ${PROJECT_NAME}::process)

add_executable(gen gen.cpp)
target_link_libraries(gen fmt::fmt ${PROJECT_NAME}::process Threads::Threads)

