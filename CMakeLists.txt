cmake_minimum_required(VERSION 3.30)

project(
  PatrickRoberts
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(PATRICK_ROBERTS_INSTALL_PRE_COMMIT_HOOKS
       "Install pre-commit hooks during configuration" OFF)

if(PATRICK_ROBERTS_INSTALL_PRE_COMMIT_HOOKS)
  include(cmake/pre-commit.cmake)
endif()

add_library(patrickroberts INTERFACE)
target_compile_features(patrickroberts INTERFACE cxx_std_23)
target_include_directories(patrickroberts
                           INTERFACE "${PROJECT_SOURCE_DIR}/include")

function(add_pr_library name source)
  add_library(${name} STATIC ${source})
  target_compile_features(${name} PRIVATE cxx_std_23)
  target_include_directories(${name} PRIVATE "${PROJECT_SOURCE_DIR}/include")
  target_link_libraries(patrickroberts INTERFACE ${name})
endfunction()

if(UNIX)
  set(file_source src/file_posix.cpp)
  set(file_native_handle_type int)
  set(file_invalid_native_handle "(-1)")
elseif(WIN32)
  set(file_source src/file_windows.cpp)
  set(file_native_handle_type void*)
  set(file_invalid_native_handle nullptr)
endif()

if(DEFINED file_source)
  add_pr_library(patrickroberts_file "${file_source}")
  target_compile_definitions(
    patrickroberts_file
    PUBLIC PR_FILE_NATIVE_HANDLE_TYPE=${file_native_handle_type}
           PR_FILE_INVALID_NATIVE_HANDLE=${file_invalid_native_handle})
endif()

if(UNIX)
  set(mapping_source src/mapping_posix.cpp)
elseif(WIN32)
  set(mapping_source src/mapping_windows.cpp)
endif()

if(DEFINED mapping_source)
  add_pr_library(patrickroberts_mapping "${mapping_source}")
  target_link_libraries(patrickroberts_mapping PUBLIC patrickroberts_file)
endif()

if(LINUX)
  set(shared_mutex_source src/shared_mutex_linux.cpp)
elseif(WIN32)
  set(shared_mutex_source src/shared_mutex_windows.cpp)
endif()

if(DEFINED shared_mutex_source)
  add_pr_library(patrickroberts_shared_mutex "${shared_mutex_source}")
endif()

add_subdirectory(examples)
