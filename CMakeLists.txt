cmake_minimum_required(VERSION 3.20)
project(small_lang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
# message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
# message(STATUS "LLVM includes: ${LLVM_INCLUDE_DIRS}")
# message(STATUS "LLVM libs: ${LLVM_LIBRARIES}")


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

# include(FetchContent)
# FetchContent_Declare(
#   replxx
#   GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
#   GIT_TAG master
#   # GIT_TAG 1f149bfe20bf6e49c1afd4154eaf0032c8c2fda2
# )
# FetchContent_MakeAvailable(replxx)

# target_link_libraries(ast_repl PRIVATE replxx)

