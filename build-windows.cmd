@echo off

echo "* Building Windows (x64) binaries..."
mkdir _build.windows.x64
pushd _build.windows.x64
cmake .. -G "Visual Studio 10 Win64" -DMP_NO_TESTS=ON
cmake --build . --config RelWithDebInfo
popd

echo "* Building Windows (x86) binaries..."
mkdir _build.windows.x86
pushd _build.windows.x86
cmake .. -G "Visual Studio 10" -DMP_NO_TESTS=ON
cmake --build . --config RelWithDebInfo
popd
