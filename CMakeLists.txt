cmake_minimum_required(VERSION 3.20)
project(small_lang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Sources
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(small_lang ${CORE_SOURCES})

# --- Includes
target_include_directories(small_lang
    PUBLIC
        "${CMAKE_SOURCE_DIR}/src"
)

# --- Compile flags
if(MSVC)
    target_compile_options(small_lang PRIVATE /W3 /permissive-)
else()
    target_compile_options(small_lang PRIVATE -Wall -Wextra -Wpedantic)
endif()
