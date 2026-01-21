cmake_minimum_required(VERSION 3.5)

project(lab_2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(spdlog REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc)
find_package(ZLIB REQUIRED)

add_executable(lab2)

target_compile_options(lab2 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    # -Werror
)

target_link_libraries(lab2 PRIVATE ICU::uc spdlog::spdlog ZLIB::ZLIB)

target_include_directories(lab2 PRIVATE include/ ${ZLIB_INCLUDE_DIRS})

target_sources(
    lab2 
    PRIVATE 
    src/main.cpp
    src/text_processor.cpp 
    src/text_transformations.cpp
    src/statistics.cpp
    src/criteria.cpp
    src/interfaces.cpp
)
