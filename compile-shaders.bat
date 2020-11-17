@echo off

REM compile shaders

REM simple shader
third-party\build\bin\shaderc.exe ^
-f shader\simple\v_simple.sc -o shader\simple\v_simple.bin ^
--platform windows --type vertex --verbose -i ./ -p vs_5_0

third-party\build\bin\shaderc.exe ^
-f shader\simple\f_simple.sc -o shader\simple\f_simple.bin ^
--platform windows --type fragment --verbose -i ./ -p ps_5_0

REM instance shader
third-party\build\bin\shaderc.exe ^
-f shader\instance\v_instance.sc -o shader\instance\v_instance.bin ^
--platform windows --type vertex --verbose -i ./ -p vs_5_0

third-party\build\bin\shaderc.exe ^
-f shader\instance\f_instance.sc -o shader\instance\f_instance.bin ^
--platform windows --type fragment --verbose -i ./ -p ps_5_0
