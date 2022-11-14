@echo off

REM Command to configure this repo for building after dependencies have been installed

cmake -B build-vs -G "Visual Studio 17 2022" ^
-DCMAKE_PREFIX_PATH=%cd%/third-party/build ^
-DAS_PRECISION_FLOAT=ON -DAS_COL_MAJOR=ON
