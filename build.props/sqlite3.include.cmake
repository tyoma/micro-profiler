include(conan)

conan_cmake_run(REQUIRES sqlite3/3.40.0
	BUILD missing
	BASIC_SETUP
	CMAKE_TARGETS
	OPTIONS sqlite3:shared=False;sqlite3:build_executable=False)
