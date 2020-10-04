#!/bin/bash

# compile shaders

./third-party/libs/bgfx/install/bin/shaderc  \
-f shader/simple/v_simple.sc -o shader/simple/v_simple.bin  \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/libs/bgfx/install/bin/shaderc \
-f shader/simple/f_simple.sc -o shader/simple/f_simple.bin \
--platform osx --type fragment --verbose -i ./ -p metal
