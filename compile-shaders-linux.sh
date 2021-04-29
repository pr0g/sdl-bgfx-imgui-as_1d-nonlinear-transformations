#!/bin/bash

# compile shaders

# simple shader
./third-party/build/bin/shaderc \
-f shader/simple/v_simple.sc -o shader/simple/v_simple.bin  \
--platform linux --type vertex --verbose -i ./

./third-party/build/bin/shaderc \
-f shader/simple/f_simple.sc -o shader/simple/f_simple.bin \
--platform linux --type fragment --verbose -i ./

# next (normal) shader
./third-party/build/bin/shaderc \
-f shader/next/v_next.sc -o shader/next/v_next.bin \
--platform linux --type vertex --verbose -i ./

./third-party/build/bin/shaderc \
-f shader/next/f_next.sc -o shader/next/f_next.bin \
--platform linux --type fragment --verbose -i ./
