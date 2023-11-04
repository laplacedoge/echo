cmake_minimum_required(VERSION 3.15)

project(echo LANGUAGES C)

add_compile_options(-g -O0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(echo STATIC echo.c echo.h conf.h)

add_subdirectory(test)
