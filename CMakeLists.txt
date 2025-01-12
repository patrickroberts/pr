cmake_minimum_required(VERSION 3.30)

project(
  PatrickRoberts
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

option(PATRICK_ROBERTS_INSTALL_PRE_COMMIT_HOOKS
       "Install pre-commit hooks during configuration" ON)
option(PATRICK_ROBERTS_USE_LIBCXX
       "Use libc++ standard library when compiling patrickroberts with Clang"
       OFF)

if(PATRICK_ROBERTS_INSTALL_PRE_COMMIT_HOOKS)
  include(cmake/pre-commit.cmake)
endif()

add_library(patrickroberts INTERFACE)
target_compile_features(patrickroberts INTERFACE cxx_std_23)
target_compile_options(
  patrickroberts
  INTERFACE
    -Wall
    -Werror
    -Wextra
    -pedantic
    $<$<CXX_COMPILER_ID:Clang>:$<$<BOOL:${PATRICK_ROBERTS_USE_LIBCXX}>:-stdlib=libc++>
    -D_GLIBCXX_DO_NOT_USE_BUILTIN_TRAITS>)
target_include_directories(patrickroberts
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

enable_testing()
add_subdirectory(test)
