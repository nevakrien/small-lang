cmake_minimum_required(VERSION 3.20)
project(small_lang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(add_default_setup target)
    target_include_directories(${target}
        PUBLIC
            "${CMAKE_SOURCE_DIR}/src"
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W3 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# --- Sources
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

add_executable(ast_repl ast_repl.cpp ${CORE_SOURCES})
add_default_setup(ast_repl)
