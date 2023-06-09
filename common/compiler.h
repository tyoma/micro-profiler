//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#if defined(_MSC_VER) && defined(_M_IX86)
	#define CC_(cc) __ ## cc
	#define _CC(cc)

#elif (defined(__GNUC__) || defined(__clang__)) && defined(__i386)
	#define CC_(cc)
	#define _CC(cc) __attribute__((cc))

#else
	#define CC_(cc)
	#define _CC(cc)

#endif

#if defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
	#define FORCE_NOINLINE __declspec(noinline)

#elif defined(__GNUC__) || defined(__clang__)
	#define FORCE_INLINE __attribute__((always_inline)) inline
	#define FORCE_NOINLINE __attribute__((noinline))

#else
	#define FORCE_INLINE inline
	#define FORCE_NOINLINE

#endif

#if defined(_MSC_VER)
	#define DISABLE_CCTOR_DEPRECATION
	#define RESTORE_CCTOR_DEPRECATION

#elif defined(__GNUC__)
	#define DISABLE_CCTOR_DEPRECATION \
		#pragma GCC diagnostic ignored "-Wdeprecated-"
	#define FORCE_INLINE __attribute__((always_inline)) inline
	#define FORCE_NOINLINE __attribute__((noinline))

#else
	#define DISABLE_CCTOR_DEPRECATION
	#define RESTORE_CCTOR_DEPRECATION

#endif
