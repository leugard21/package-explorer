cmake_minimum_required(VERSION 3.22)
project(package-explorer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(package-explorer
    src/main.cpp
    src/DummyPackageManager.cpp
    src/PacmanPackageManager.cpp
)

target_include_directories(package-explorer PRIVATE include)

find_package(Curses REQUIRED)
target_link_libraries(package-explorer PRIVATE ${CURSES_LIBRARIES})

install(TARGETS package-explorer RUNTIME DESTINATION bin)
