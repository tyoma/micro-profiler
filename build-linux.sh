#if [ -z "$NDK" ]
#then
#	echo No Android NDK is installed...
#else
#	echo Building Android binaries...
#	mkdir _build.android.arm
#	cd _build.android.arm
#	cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_STL=c++_static -DANDROID_PLATFORM=16
#	make -j4
#	cd ..
#fi

echo "Building Linux (x86_64) binaries..."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m64" -DCMAKE_C_FLAGS="-m64" -DMP_NO_TESTS=ON -S . -B _build.linux.x64
cmake --build _build.linux.x64 --config Release --parallel 6

echo "Building Linux (x86) binaries..."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m32" -DCMAKE_C_FLAGS="-m32" -DMP_NO_TESTS=ON -S . -B _build.linux.x86
cmake --build _build.linux.x86 --config Release --parallel 6
