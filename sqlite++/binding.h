//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "expression.h"

namespace micro_profiler
{
	namespace sql
	{
		inline void bind_parameter(sqlite3_stmt &statement, int value, unsigned int index)
		{	sqlite3_bind_int(&statement, index, value);	}

		inline void bind_parameter(sqlite3_stmt &statement, const std::string &value, unsigned int index)
		{	sqlite3_bind_text(&statement, index, value.c_str(), -1, SQLITE_TRANSIENT);	}

		template <typename T, typename F>
		inline void bind_parameters(sqlite3_stmt &/*statement*/, const column<T, F> &/*e*/, unsigned int &/*index*/)
		{	}

		template <typename T>
		inline void bind_parameters(sqlite3_stmt &statement, const parameter<T> &e, unsigned int &index)
		{	bind_parameter(statement, e.object, index++);	}

		template <typename L, typename R>
		inline void bind_parameters(sqlite3_stmt &statement, const operator_<L, R> &e, unsigned int &index)
		{	bind_parameters(statement, e.lhs, index), bind_parameters(statement, e.rhs, index);	}

		template <typename E>
		inline void bind_parameters(sqlite3_stmt &statement, const E &e)
		{
			auto index = 1u;

			bind_parameters(statement, e, index);
		}
	}
}
