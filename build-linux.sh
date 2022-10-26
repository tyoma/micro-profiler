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
mkdir _build.linux.x64
cd _build.linux.x64
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m64" -DCMAKE_C_FLAGS="-m64" -DMP_NO_TESTS=ON
make -j4
cd ..

echo "Building Linux (x86) binaries..."
mkdir _build.linux.x86
cd _build.linux.x86
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-m32" -DCMAKE_C_FLAGS="-m32" -DMP_NO_TESTS=ON
make -j4
cd ..
