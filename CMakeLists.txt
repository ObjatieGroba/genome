cmake_minimum_required(VERSION 3.10)

project(genome)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(src)

file(GLOB SOURCES src/*.cpp src/*.ui)
add_executable(genome ${SOURCES})

find_package(Qt5Widgets REQUIRED)

target_link_libraries(genome pthread)
target_link_libraries(genome Qt5::Widgets)
