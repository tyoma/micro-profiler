echo Building Android binaries...
mkdir _build.android.arm
cd _build.android.arm
cmake .. -DCMAKE_TOOLCHAIN=/mnt/c/android/android-ndk-r20-linux/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_STL=c++_static
make -j4
cd ..

echo "Building Linux (x86_64) binaries..."
mkdir _build.linux.x64
cd _build.linux.x64
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m64" -DCMAKE_C_FLAGS="-m64"
make -j4
cd ..

echo "Building Linux (x86) binaries..."
mkdir _build.linux.x86
cd _build.linux.x86
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m32" -DCMAKE_C_FLAGS="-m32"
make -j4
cd ..
