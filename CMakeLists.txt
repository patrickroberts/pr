cmake_minimum_required(VERSION 3.30)

project(
  PatrickRoberts
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

option(PATRICK_ROBERTS_INSTALL_PRE_COMMIT_HOOKS
       "Install pre-commit hooks during configuration" OFF)

if(PATRICK_ROBERTS_INSTALL_PRE_COMMIT_HOOKS)
  include(cmake/pre-commit.cmake)
endif()

add_library(patrickroberts INTERFACE)
target_compile_features(patrickroberts INTERFACE cxx_std_20)
target_include_directories(patrickroberts
                           INTERFACE "${PROJECT_SOURCE_DIR}/include")
