@echo off

REM compiler shaders

REM simple shader
third-party\build\bin\shaderc.exe ^
-f shader\simple\v_simple.sc -o shader\simple\v_simple.bin ^
--platform windows --type vertex --verbose -i ./ -p vs_5_0

third-party\build\bin\shaderc.exe ^
-f shader\simple\f_simple.sc -o shader\simple\f_simple.bin ^
--platform windows --type fragment --verbose -i ./ -p ps_5_0

REM next (normal) shader
third-party\build\bin\shaderc.exe ^
-f shader\next\v_next.sc -o shader\next\v_next.bin ^
--platform windows --type vertex --verbose -i ./ -p vs_5_0

third-party\build\bin\shaderc.exe ^
-f shader\next\f_next.sc -o shader\next\f_next.bin ^
--platform windows --type fragment --verbose -i ./ -p ps_5_0

REM basic (normal) lighting shader
third-party\build\bin\shaderc.exe ^
-f shader\basic-lighting\v_basic.sc -o shader\basic-lighting\v_basic.bin ^
--platform windows --type vertex --verbose -i ./ -p vs_5_0

third-party\build\bin\shaderc.exe ^
-f shader/basic-lighting/f_basic.sc -o shader/basic-lighting/f_basic.bin ^
--platform windows --type fragment --verbose -i ./ -p ps_5_0

