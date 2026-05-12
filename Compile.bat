@echo off
rem Сборка Clip1C под Windows (x86 + x64).

cmake -E remove_directory build32W
cmake -E remove_directory build64W
if exist bin\Release\Clip1CWin32.dll del bin\Release\Clip1CWin32.dll
if exist bin\Release\Clip1CWin64.dll del bin\Release\Clip1CWin64.dll

mkdir build32W
pushd build32W
cmake .. -A Win32 -DTARGET_PLATFORM_32:BOOL=ON
cmake --build . --config Release
popd

mkdir build64W
pushd build64W
cmake .. -A x64 -DTARGET_PLATFORM_32:BOOL=OFF
cmake --build . --config Release
popd
