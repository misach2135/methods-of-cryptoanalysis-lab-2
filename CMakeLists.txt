cmake_minimum_required(VERSION 3.2)

project(lab_2)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lab2)

target_include_directories(lab2 PRIVATE include/)

target_sources(lab2 PRIVATE src/main.cpp src/text_processor.cpp)