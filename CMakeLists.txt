cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 23)

project("fish_simulation")

find_package(raylib 3.0 REQUIRED)

add_executable(fish_simulation "main.cpp" "swimming.cpp")

target_link_libraries(fish_simulation raylib)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(fish_simulation "-framework IOKit")
    target_link_libraries(fish_simulation "-framework Cocoa")
    target_link_libraries(fish_simulation "-framework OpenGL")
endif()
