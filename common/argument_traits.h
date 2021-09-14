//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <functional>

namespace micro_profiler
{
	template <typename F>
	struct arguments_pack;

	template <>
	struct arguments_pack<void ()>
	{
		enum {	count = 0,	};
	};

	template <typename T1>
	struct arguments_pack<void (T1)>
	{
		enum {	count = 1,	};
		typedef T1 argument_1_type;
	};

	template <typename T1, typename T2>
	struct arguments_pack<void (T1, T2)>
	{
		enum {	count = 2,	};
		typedef T1 argument_1_type;
		typedef T2 argument_2_type;
	};

	template<typename F, typename Ret>
	arguments_pack<void ()> argtype_helper(Ret (F::*)());

	template<typename F, typename Ret>
	arguments_pack<void ()> argtype_helper(Ret (F::*)() const);

	template<typename F, typename Ret, typename T1>
	arguments_pack<void (T1)> argtype_helper(Ret (F::*)(T1));

	template<typename F, typename Ret, typename T1>
	arguments_pack<void (T1)> argtype_helper(Ret (F::*)(T1) const);

	template<typename F, typename Ret, typename T1, typename T2>
	arguments_pack<void (T1, T2)> argtype_helper(Ret (F::*)(T1, T2));

	template<typename F, typename Ret, typename T1, typename T2>
	arguments_pack<void (T1, T2)> argtype_helper(Ret (F::*)(T1, T2) const);

	template <typename F>
	struct argument_traits
	{
		typedef decltype(argtype_helper(&F::operator())) types;
	};
}
