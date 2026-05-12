@echo off
rem Clip1C build for Windows (x86 + x64) and pack to AddIn.zip.

cmake -E remove_directory build32W
cmake -E remove_directory build64W
if exist bin32\Release\Clip1CWin32.dll del bin32\Release\Clip1CWin32.dll
if exist bin64\Release\Clip1CWin64.dll del bin64\Release\Clip1CWin64.dll

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

if not exist bin32\Release\Clip1CWin32.dll (
    echo [ERROR] x86 build failed
    exit /b 1
)
if not exist bin64\Release\Clip1CWin64.dll (
    echo [ERROR] x64 build failed
    exit /b 1
)

rem --- Detect Python (optional) -------------------------------------------
set "PYTHON="
where python.exe  >nul 2>&1 && set "PYTHON=python.exe"
if not defined PYTHON where python3.exe >nul 2>&1 && set "PYTHON=python3.exe"
if not defined PYTHON where py.exe      >nul 2>&1 && set "PYTHON=py.exe"
if not defined PYTHON (
    echo [WARN] Python not found in PATH, skipping AddIn.zip packaging.
    echo        DLLs are in bin32\Release\ and bin64\Release\.
    exit /b 0
)

rem --- Pack AddIn.zip -----------------------------------------------------
echo ==^> Packing AddIn.zip
%PYTHON% tools\make_bundle.py --artifacts-dir . --output AddIn.zip
if errorlevel 1 (
    echo [ERROR] make_bundle.py failed
    exit /b 1
)

echo.
echo ==^> Done: AddIn.zip
exit /b 0
