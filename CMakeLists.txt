cmake_minimum_required(VERSION 3.23)

project(sdl-bgfx-imgui-as_1d-nonlinear-transformations LANGUAGES CXX)

option(SDL_BGFX_IMGUI_ENABLE_TESTING "Enable tests for the project" OFF)

find_package(SDL2 REQUIRED CONFIG)
find_package(bgfx REQUIRED CONFIG)
find_package(imgui.cmake REQUIRED CONFIG)

include(FetchContent)

FetchContent_Declare(
  as-camera-input-sdl
  GIT_REPOSITORY https://github.com/pr0g/as-camera-input-sdl.git
  GIT_TAG c83e6ae2a0727a6e8f7f40ec277f49683f74771a)
FetchContent_Declare(
  EasyIterator
  GIT_REPOSITORY https://github.com/TheLartians/EasyIterator.git
  GIT_TAG f3f89399ba8bca617040c1503f4f9fe3582a6abd)
FetchContent_Declare(
  thh-bgfx-debug
  GIT_REPOSITORY https://github.com/pr0g/thh-bgfx-debug
  GIT_TAG 15238e3adb87ac3a0590afcdc24510e98326de26)
FetchContent_Declare(
  hierarchy
  GIT_REPOSITORY https://github.com/pr0g/hierarchy.git
  GIT_TAG 9d1dbadb0c9822ae983de2a6196ccce02506d2e3)
FetchContent_Declare(
  nlohmann-json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG 0457de21cffb298c22b629e538036bfeb96130b7)
FetchContent_Declare(
  as
  GIT_REPOSITORY https://github.com/pr0g/as.git
  GIT_TAG 9355e7572b354cdce950cedd131acd68a910804c)
FetchContent_Declare(
  bec
  GIT_REPOSITORY https://github.com/pr0g/bit-field-enum-class.git
  GIT_TAG e68e6e1ce87335c14b005e410c68160ec8ea8aa6)

FetchContent_MakeAvailable(
  as-camera-input-sdl
  as
  EasyIterator
  thh-bgfx-debug
  hierarchy
  nlohmann-json
  bec)

add_executable(${PROJECT_NAME})
target_sources(
  ${PROJECT_NAME}
  PRIVATE main.cpp
          sdl-imgui/imgui_impl_sdl2.cpp
          bgfx-imgui/imgui_impl_bgfx.cpp
          smooth-line.cpp
          fps.cpp
          hierarchy-imgui.cpp
          bgfx-helpers.cpp
          debug.cpp
          list.cpp
          render-thing.cpp
          marching-cubes/marching-cubes.cpp
          csg/csg.cpp
          scenes/marching-cube-scene.cpp
          scenes/transforms-scene.cpp
          scenes/hierarchy-scene.cpp
          scenes/simple-camera-scene.cpp
          scenes/globe-scene.cpp
          scenes/arcball-scene.cpp
          scenes/list-scene.cpp
          scenes/rubiks-cube-scene.cpp
          scenes/csg-scene.cpp
          math-utils.cpp)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE SDL2::SDL2
          SDL2::SDL2main
          imgui.cmake::imgui.cmake
          as-camera-input-sdl
          EasyIterator
          thh-bgfx-debug
          hierarchy
          nlohmann_json::nlohmann_json
          as
          bec)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

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

if(SDL_BGFX_IMGUI_ENABLE_TESTING)
  find_package(Catch2 REQUIRED CONFIG)
  include(CTest)
  add_executable(${PROJECT_NAME}-list-test)
  target_sources(${PROJECT_NAME}-list-test PRIVATE list.cpp list.test.cpp)
  target_link_libraries(${PROJECT_NAME}-list-test Catch2::Catch2WithMain as
                        EasyIterator thh-bgfx-debug as-camera-input-sdl)
  target_compile_features(${PROJECT_NAME}-list-test PRIVATE cxx_std_20)
  target_include_directories(${PROJECT_NAME}-list-test
                             PRIVATE ${CMAKE_SOURCE_DIR})
  target_compile_definitions(
    ${PROJECT_NAME}-list-test
    PRIVATE $<$<BOOL:${AS_PRECISION_FLOAT}>:AS_PRECISION_FLOAT>
            $<$<BOOL:${AS_PRECISION_DOUBLE}>:AS_PRECISION_DOUBLE>
            $<$<BOOL:${AS_COL_MAJOR}>:AS_COL_MAJOR>
            $<$<BOOL:${AS_ROW_MAJOR}>:AS_ROW_MAJOR>)
  add_test(NAME "list tests" COMMAND ${PROJECT_NAME}-list-test)
endif()
