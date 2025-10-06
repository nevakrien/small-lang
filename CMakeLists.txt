cmake_minimum_required(VERSION 3.20)
project(small_lang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force clang++ toolchain (use the one that shipped with LLVM)
set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
set(CMAKE_LINKER lld CACHE STRING "" FORCE)

# Add libc++ flags — make sure to use both runtime and ABI
add_compile_options(-stdlib=libc++ -Wall -Wextra)
add_link_options(-stdlib=libc++ -fuse-ld=lld)

# --- LLVM setup ---
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")


llvm_map_components_to_libnames(LLVM_LIBS
    core
    # support
    # irreader
    # executionengine
)
add_compile_definitions(PUBLIC ${LLVM_DEFINITIONS})
link_libraries(${LLVM_LIBS})
include_directories(SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})


# --- Sources
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

include_directories(PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_executable(ast_repl ast_repl.cpp ${CORE_SOURCES})
add_executable(test_compile test_compile.cpp)
add_executable(gpt_test gpt_test.cpp)

# include(FetchContent)
# FetchContent_Declare(
#   replxx
#   GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
#   GIT_TAG master
#   # GIT_TAG 1f149bfe20bf6e49c1afd4154eaf0032c8c2fda2
# )
# FetchContent_MakeAvailable(replxx)

# target_link_libraries(ast_repl PRIVATE replxx)

