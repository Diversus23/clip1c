#!/bin/sh
#
# Сборка Clip1C под Linux и macOS.
#   ./build.sh        — сборка обеих архитектур (где применимо)
#   ./build.sh 32     — только 32-битная сборка (Linux)
#   ./build.sh 64     — только 64-битная сборка
#   ./build.sh 0      — очистить каталоги build*
#

set -e

OS=$(uname -s)

if [ -n "$1" ]; then
  build32=0
  build64=0
  clear=0
  case "$1" in
    32) build32=1 ;;
    64) build64=1 ;;
    0)  clear=1 ;;
    *)  echo "Использование: $0 [32|64|0]"; exit 1 ;;
  esac
else
  build32=0
  build64=1
  clear=1
fi

# 32-битная сборка поддерживается только на Linux
if [ "$OS" = "Darwin" ] && [ $build32 -eq 1 ]; then
  echo "32-битная сборка не поддерживается на macOS"
  exit 1
fi

# На Linux по умолчанию собираем обе архитектуры
if [ "$OS" != "Darwin" ] && [ -z "$1" ]; then
  build32=1
fi

if [ $clear -eq 1 ]; then
  cmake -E remove_directory build32L
  cmake -E remove_directory build64L
fi

if [ $build32 -eq 1 ]; then
  cmake -E echo "Build 32"
  if [ ! -d build32L ]; then
    cmake -E make_directory build32L
    (cd build32L && cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=ON ..)
  fi
  cmake --build build32L
fi

if [ $build64 -eq 1 ]; then
  cmake -E echo "Build 64"
  if [ ! -d build64L ]; then
    cmake -E make_directory build64L
    (cd build64L && cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF ..)
  fi
  cmake --build build64L
fi
