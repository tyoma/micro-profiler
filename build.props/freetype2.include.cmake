include(conan)

conan_cmake_run(REQUIRES freetype/2.12.1
	BUILD missing
	BASIC_SETUP
	CMAKE_TARGETS
	OPTIONS freetype:shared=False;freetype:with_brotli=False;freetype:with_bzip2=False;freetype:with_png=False;freetype:with_zlib=False)
