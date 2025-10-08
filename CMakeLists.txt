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

add_compile_definitions(PUBLIC ${LLVM_DEFINITIONS})
include_directories(SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    orcjit
    native
    support
)


link_libraries(${LLVM_LIBS})


# --- Sources
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

include_directories(PUBLIC "${CMAKE_SOURCE_DIR}/src")



add_library(small_lang SHARED ${CORE_SOURCES})
find_library(ZSTD_SHARED_LIB zstd REQUIRED)
target_link_libraries(small_lang PRIVATE ${ZSTD_SHARED_LIB})

add_executable(test_compile test_compile.cpp)
target_link_libraries(test_compile PRIVATE small_lang)

add_executable(ast_repl ast_repl.cpp)
target_link_libraries(test_compile PRIVATE small_lang)


add_executable(small small.cpp)
target_link_libraries(small PRIVATE small_lang)



# include(FetchContent)
# FetchContent_Declare(
#   replxx
#   GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx.git
#   GIT_TAG master
#   # GIT_TAG 1f149bfe20bf6e49c1afd4154eaf0032c8c2fda2
# )
# FetchContent_MakeAvailable(replxx)

# target_link_libraries(ast_repl PRIVATE replxx)


# if(LLVM_MAKE_SHARED)
  # message(STATUS "Forcing manual clang++ link to build libMY_LLVM.so")

  # # Expand LLVM_LIBS to absolute .a paths
  # set(LLVM_ARCHIVES "")
  # foreach(lib IN LISTS LLVM_LIBS)
  #   if(NOT IS_ABSOLUTE "${lib}")
  #     list(APPEND LLVM_ARCHIVES "${LLVM_LIBRARY_DIR}/lib${lib}.a")
  #   else()
  #     list(APPEND LLVM_ARCHIVES "${lib}")
  #   endif()
  # endforeach()

  # set(MY_LLVM_SO "${CMAKE_BINARY_DIR}/libMY_LLVM.so")

  # add_custom_command(
  #     OUTPUT ${MY_LLVM_SO}
  #     COMMAND clang++ -fPIC -shared -o ${MY_LLVM_SO}
  #             -Wl,--whole-archive
  #             ${LLVM_ARCHIVES}
  #             -Wl,--no-whole-archive
  #             -Wl,--allow-multiple-definition
  #             -fuse-ld=lld -rtlib=compiler-rt -nostdlib++
  #             -lc++ -lc++abi -ldl -lm -lz -pthread
  #     DEPENDS ${LLVM_ARCHIVES}
  #     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  #     COMMENT "Manually linking LLVM static libs into shared libMY_LLVM.so"
  #     VERBATIM
  # )

  # add_custom_target(MY_LLVM DEPENDS ${MY_LLVM_SO})

  # add_library(MY_LLVM_SO SHARED IMPORTED GLOBAL)
  # set_target_properties(MY_LLVM_SO PROPERTIES IMPORTED_LOCATION ${MY_LLVM_SO})
  # add_dependencies(MY_LLVM_SO MY_LLVM)

  # link_libraries(MY_LLVM_SO)
# endif()
