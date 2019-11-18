@echo off
set NDK=C:\android\android-ndk-r20-linux\
set WSLENV=NDK/p:CMAKE_TOOLCHAIN_FILE/p

echo 1. Building Android binaries...
mkdir %~dp0\_build.android.arm
cd %~dp0\_build.android.arm
wsl cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_STL=c++_static
wsl make -j4

echo 2. Building Linux (x86_64) binaries...
mkdir %~dp0\_build.linux.x86_64
cd %~dp0\_build.linux.x86_64
wsl cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m64" -DCMAKE_C_FLAGS="-m64"
wsl make -j4

echo 3. Building Linux (x86) binaries...
mkdir %~dp0\_build.linux.x86
cd %~dp0\_build.linux.x86
wsl cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m32" -DCMAKE_C_FLAGS="-m32"
wsl make -j4

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
