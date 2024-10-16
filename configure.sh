#!/bin/bash

# Commands to configure this repo for building after dependencies have been installed

cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_PREFIX_PATH="$(pwd)/third-party/build" \
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
-DAS_PRECISION_FLOAT=ON -DAS_COL_MAJOR=ON \
-DSDL_BGFX_IMGUI_ENABLE_TESTING=ON

cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release \
-DCMAKE_PREFIX_PATH="$(pwd)/third-party/build" \
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
-DAS_PRECISION_FLOAT=ON -DAS_COL_MAJOR=ON
