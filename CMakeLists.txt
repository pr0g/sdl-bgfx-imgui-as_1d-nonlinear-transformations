cmake_minimum_required(VERSION 3.15)

project(sdl-bgfx-imgui-as_1d-nonlinear-transformations LANGUAGES CXX)

find_package(SDL2 REQUIRED CONFIG)
find_package(bgfx REQUIRED CONFIG)
find_package(imgui REQUIRED CONFIG)

include(FetchContent)

FetchContent_Declare(
    as-camera
    GIT_REPOSITORY https://github.com/pr0g/as-camera.git
    GIT_TAG        origin/camera-experiments)
FetchContent_Declare(
    bec
    GIT_REPOSITORY https://github.com/pr0g/Bit-Field-Enum-Class.git
    GIT_TAG        origin/main)
FetchContent_Declare(
    EasyIterator
    GIT_REPOSITORY https://github.com/TheLartians/EasyIterator.git)

FetchContent_MakeAvailable(as-camera EasyIterator bec)

add_executable(${PROJECT_NAME})
target_sources(
    ${PROJECT_NAME} PRIVATE
    main.cpp
    sdl-imgui/imgui_impl_sdl.cpp
    bgfx-imgui/imgui_impl_bgfx.cpp
    as-camera-sdl/as-camera-sdl.cpp
    debug-lines.cpp
    smooth-line.cpp
    fps.cpp)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    bgfx::bgfx
    imgui::imgui
    as-camera
    EasyIterator
    bec)

# required by as library
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE -DAS_PRECISION_FLOAT -DAS_COL_MAJOR)

target_compile_features(
    ${PROJECT_NAME} PRIVATE cxx_std_17)

# required for project when using visual studio generator
set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if (WIN32)
# copy the SDL2.dll to the same folder as the executable
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
endif()
