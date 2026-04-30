cmake_minimum_required(VERSION 3.20)
project(FreeGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# Dear ImGui requires source files to be compiled
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(DBUS REQUIRED dbus-1)

include_directories(include)
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)

file(GLOB SOURCES "src/*.cpp")

add_executable(FreeGen ${SOURCES} ${IMGUI_SOURCES})

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vert.spv ${CMAKE_CURRENT_SOURCE_DIR}/shaders/frag.spv
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fsr_upscale.spv ${CMAKE_CURRENT_SOURCE_DIR}/shaders/passthrough.spv
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/shaders)

target_include_directories(FreeGen PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${PIPEWIRE_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(FreeGen PRIVATE
    Vulkan::Vulkan
    Threads::Threads
    ${PIPEWIRE_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${DBUS_LIBRARIES}
)
