#!/bin/bash

# compile shaders

# simple shader
./third-party/build/bin/shaderc \
-f shader/simple/v_simple.sc -o shader/simple/v_simple.bin  \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/build/bin/shaderc \
-f shader/simple/f_simple.sc -o shader/simple/f_simple.bin \
--platform osx --type fragment --verbose -i ./ -p metal

# next (normal) shader
./third-party/build/bin/shaderc \
-f shader/next/v_next.sc -o shader/next/v_next.bin \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/build/bin/shaderc \
-f shader/next/f_next.sc -o shader/next/f_next.bin \
--platform osx --type fragment --verbose -i ./ -p metal

# basic (normal) lighting shader
./third-party/build/bin/shaderc \
-f shader/basic-lighting/v_basic.sc -o shader/basic-lighting/v_basic.bin \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/build/bin/shaderc \
-f shader/basic-lighting/f_basic.sc -o shader/basic-lighting/f_basic.bin \
--platform osx --type fragment --verbose -i ./ -p metal

# basic (normal) lighting shader with vertex colors
./third-party/build/bin/shaderc \
-f shader/basic-lighting-vert-col/v_basic_vc.sc -o shader/basic-lighting-vert-col/v_basic_vc.bin \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/build/bin/shaderc \
-f shader/basic-lighting-vert-col/f_basic_vc.sc -o shader/basic-lighting-vert-col/f_basic_vc.bin \
--platform osx --type fragment --verbose -i ./ -p metal
