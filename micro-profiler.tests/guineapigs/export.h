#pragma once

#if defined(__clang__) || defined(__GNUC__)
	#define PUBLIC __attribute__ ((visibility ("default")))
#else
	#define PUBLIC __declspec(dllexport)
#endif
