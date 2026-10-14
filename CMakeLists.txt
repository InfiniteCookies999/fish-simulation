cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 23)

project("fish_simulation")

include(FetchContent)

# Tell raylib to select web platform if compiling for web assembly.
if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "" FORCE)
endif()

# Fetch raylib source and build it for the current toolchain.
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)

FetchContent_MakeAvailable(raylib)

add_executable(fish_simulation "main.cpp" "swimming.cpp" "more_maths.cpp"
    "libs/rlImGui/rlImGui.cpp"
    "libs/imgui/imgui.cpp"
    "libs/imgui/imgui_draw.cpp"
    "libs/imgui/imgui_widgets.cpp"
    "libs/imgui/imgui_tables.cpp"
)

target_include_directories (fish_simulation SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui")
target_include_directories (fish_simulation SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libs/rlImGui")

target_link_libraries(fish_simulation raylib)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(fish_simulation "-framework IOKit")
    target_link_libraries(fish_simulation "-framework Cocoa")
    target_link_libraries(fish_simulation "-framework OpenGL")
endif()

if(EMSCRIPTEN)
    # Give the executable target a .html suffix.
    set_target_properties(fish_simulation PROPERTIES
        SUFFIX ".html"
        OUTPUT_NAME "index"
    )

    target_link_options(fish_simulation PRIVATE
        # Allow memory to grow at runtime.
        "-sALLOW_MEMORY_GROWTH=1"
        # Use our own shell file to display the game.
        "--shell-file"
        "${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
        # Something is relying on emscripten_sleep() possibly the frame limiting.
        "-sASYNCIFY"
        # Package the game assets.
        "--preload-file"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
    )
endif()
