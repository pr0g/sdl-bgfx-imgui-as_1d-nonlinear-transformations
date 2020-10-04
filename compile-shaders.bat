@echo off

REM compile shaders

REM normal shader
third-party\libs\bgfx\install\bin\shaderc.exe ^
-f shader\simple\v_simple.sc -o shader\simple\v_simple.bin ^
--platform windows --type vertex --verbose -i ./ -p vs_5_0

third-party\libs\bgfx\install\bin\shaderc.exe ^
-f shader\simple\f_simple.sc -o shader\simple\f_simple.bin ^
--platform windows --type fragment --verbose -i ./ -p ps_5_0
