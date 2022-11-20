@echo off

echo "* Building Windows (x64) binaries..."
cmake -G "Visual Studio 10 Win64" -DMP_NO_TESTS=ON -S . -B _build.windows.x64
cmake --build _build.windows.x64 --config Release --parallel 6

echo "* Building Windows (x86) binaries..."
cmake -G "Visual Studio 10" -DMP_NO_TESTS=ON -S . -B _build.windows.x86
cmake --build _build.windows.x86 --config Release --parallel 6
