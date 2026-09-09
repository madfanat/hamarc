cmake_minimum_required(VERSION 3.24)

project(hamarc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hamarc src/main.cc)
target_include_directories(hamarc PRIVATE "${PROJECT_SOURCE_DIR}/src")

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
