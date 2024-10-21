cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0135 NEW)

project(third-party)

include(ExternalProject)

option(SDL_BGFX_IMGUI_ENABLE_TESTING "Enable testing" OFF)

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT isMultiConfig)
  if(NOT CMAKE_BUILD_TYPE)
    # set Debug as default build type if one is not provided (match CMake
    # default behavior)
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE)
  endif()
  # give different build types their own folder with single config generators
  set(build_type_dir ${CMAKE_BUILD_TYPE})
  # pass through build type arg to ExternalProject_Add command
  set(build_type_arg -DCMAKE_BUILD_TYPE=$<CONFIG>)
endif()

ExternalProject_Add(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.2
  GIT_SHALLOW TRUE
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/SDL2/build/${build_type_dir}
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

ExternalProject_Add(
  bgfx
  GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
  GIT_TAG v1.128.8808-482
  GIT_SHALLOW TRUE
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bgfx/build/${build_type_dir}
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             -DBGFX_BUILD_EXAMPLES=OFF)

ExternalProject_Add(
  imgui.cmake
  GIT_REPOSITORY https://github.com/pr0g/imgui.cmake.git
  GIT_TAG abf77e443a0d23b6d874dbbb42bbd02009562a9a
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/imgui/build/${build_type_dir}
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             "$<$<CONFIG:Debug>:-DCMAKE_DEBUG_POSTFIX=d>"
             -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=ON)

if(SDL_BGFX_IMGUI_ENABLE_TESTING)
  ExternalProject_Add(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Catch2/build/${build_type_dir}
    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    CMAKE_CACHE_ARGS -DCMAKE_DEBUG_POSTFIX:STRING=d)
endif()
