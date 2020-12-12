#!/bin/bash

# compile shaders

# simple shader
./third-party/build/bin/shaderc \
-f shader/simple/v_simple.sc -o shader/simple/v_simple.bin  \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/build/bin/shaderc \
-f shader/simple/f_simple.sc -o shader/simple/f_simple.bin \
--platform osx --type fragment --verbose -i ./ -p metal

# instance shader
./third-party/build/bin/shaderc \
-f shader/instance/v_instance.sc -o shader/instance/v_instance.bin  \
--platform osx --type vertex --verbose -i ./ -p metal

./third-party/build/bin/shaderc \
-f shader/instance/f_instance.sc -o shader/instance/f_instance.bin \
--platform osx --type fragment --verbose -i ./ -p metal
