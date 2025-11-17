cmake_minimum_required(VERSION 3.22)

project(package-explorer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(package-explorer
    src/main.cpp
)

find_package(Curses REQUIRED)
target_link_libraries(package-explorer PRIVATE ${CURSES_LIBRARIES})
target_include_directories(package-explorer PRIVATE
    ${CURSES_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
