@echo off
set NDK=C:\android\android-ndk-r20-linux\
set WSLENV=NDK/p:CMAKE_TOOLCHAIN_FILE/p

wsl bash ./build.sh

echo 4. Building Windows (x64) binaries...
mkdir %~dp0\_build.windows.x64
cd %~dp0\_build.windows.x64
cmake .. -G "Visual Studio 10 Win64"
cmake --build . --config RelWithDebInfo

echo 5. Building Windows (x86) binaries...
mkdir %~dp0\_build.windows.x86
cd %~dp0\_build.windows.x86
cmake .. -G "Visual Studio 10"
cmake --build . --config RelWithDebInfo
