cmake_minimum_required(VERSION 3.15)

project(sdl-bgfx-imgui-as_1d-nonlinear-transformations LANGUAGES CXX)

find_package(SDL2 REQUIRED CONFIG)
find_package(bgfx REQUIRED CONFIG)
find_package(imgui REQUIRED CONFIG)
find_package(OpenMesh REQUIRED CONFIG)

include(FetchContent)

FetchContent_Declare(
  as-camera-input
  GIT_REPOSITORY https://github.com/pr0g/as-camera-input.git
  GIT_TAG a713cee2318b284a7ff8b9d400deea3dbbd49e2d)
FetchContent_Declare(
  EasyIterator
  GIT_REPOSITORY https://github.com/TheLartians/EasyIterator.git
  GIT_TAG aab0c0d8fd17708c64522408d9b304729dbc3a3f)
FetchContent_Declare(
  thh-bgfx-debug
  GIT_REPOSITORY https://github.com/pr0g/thh-bgfx-debug
  GIT_TAG 2427504fb9ce2895c2d8eac7559fd67402783d20)
FetchContent_Declare(
  hierarchy
  GIT_REPOSITORY https://github.com/pr0g/hierarchy.git
  GIT_TAG 49703301f6e922733a14666562ef8f954b9974b7)
FetchContent_Declare(
  nlohmann-json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG e10a3fac8a255433146e3f06a703dc110fc3c3da)

FetchContent_MakeAvailable(as-camera-input EasyIterator thh-bgfx-debug
                           hierarchy nlohmann-json)

add_executable(${PROJECT_NAME})
target_sources(
  ${PROJECT_NAME}
  PRIVATE main.cpp
          sdl-imgui/imgui_impl_sdl.cpp
          bgfx-imgui/imgui_impl_bgfx.cpp
          smooth-line.cpp
          fps.cpp
          hierarchy-imgui.cpp
          bgfx-helpers.cpp
          debug.cpp
          marching-cubes/marching-cubes.cpp
          voronoi/voronoi.cpp
          scenes/marching-cube-scene.cpp
          scenes/transforms-scene.cpp
          scenes/hierarchy-scene.cpp
          scenes/simple-camera-scene.cpp
          scenes/voronoi-scene.cpp
          sdl-camera-input.cpp
          math-utils.cpp)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE SDL2::SDL2
          SDL2::SDL2main
          bgfx::bgfx
          bgfx::bx
          imgui::imgui
          OpenMeshCore
          as-camera-input
          EasyIterator
          thh-bgfx-debug
          hierarchy
          nlohmann_json::nlohmann_json)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# required for project when using visual studio generator
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                                 ${CMAKE_SOURCE_DIR})

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE $<$<BOOL:${AS_PRECISION_FLOAT}>:AS_PRECISION_FLOAT>
          $<$<BOOL:${AS_PRECISION_DOUBLE}>:AS_PRECISION_DOUBLE>
          $<$<BOOL:${AS_COL_MAJOR}>:AS_COL_MAJOR>
          $<$<BOOL:${AS_ROW_MAJOR}>:AS_ROW_MAJOR>)

if(WIN32)
  # copy the SDL2.dll to the same folder as the executable
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
endif()
