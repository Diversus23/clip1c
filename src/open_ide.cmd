@echo off

cmake -E echo Set Visual Studio version and toolset
SET VS_VER=15
SET VS_YEAR=2017
SET VS_TOOLSET=v141_xp
SET BUILD_CONFIG=Debug
SET ARCH=Win32

cmake -S ./ -B build_ide -G "Visual Studio %VS_VER% %VS_YEAR%" -A %ARCH% -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% -DCMAKE_GENERATOR_TOOLSET=%VS_TOOLSET%
cmake --open build_ide