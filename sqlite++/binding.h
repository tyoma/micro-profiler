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
#include "statement.h"
#include "types.h"

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		struct field_binder
		{
			template <typename U>
			void operator ()(U)
			{	}

			template <typename FieldT, typename U>
			void operator ()(FieldT U::*field, const char *)
			{	statement_.bind(index++, item.*field);	}

			template <typename U>
			void operator ()(U, const char *)
			{	}

			statement &statement_;
			const T &item;
			int index;
		};

		template <typename T>
		struct primary_key_binder
		{
			template <typename U>
			void operator ()(U)
			{	}

			template <typename U, typename F>
			void operator ()(const primary_key<U, F> &field, const char *)
			{	item.*field.field = static_cast<F>(sqlite3_last_insert_rowid(&connection));	}

			template <typename U>
			void operator ()(U, const char *)
			{	}

			sqlite3 &connection;
			T &item;
		};



		template <typename T, typename F>
		inline void bind_parameters(statement &/*statement_*/, const column<T, F> &/*e*/, unsigned int &/*index*/)
		{	}

		template <typename T>
		inline void bind_parameters(statement &statement_, const parameter<T> &e, unsigned int &index)
		{	statement_.bind(index++, e.object);	}

		template <typename L, typename R>
		inline void bind_parameters(statement &statement_, const operator_<L, R> &e, unsigned int &index)
		{	bind_parameters(statement_, e.lhs, index), bind_parameters(statement_, e.rhs, index);	}


		template <typename E>
		inline void bind_parameters(statement &statement_, const E &e)
		{
			auto index = 1u;

			bind_parameters(statement_, e, index);
		}

		template <typename T, typename T2>
		inline void bind_fields(statement &statement_, T2 &record)
		{
			field_binder<T> b = {	statement_, record, 1	};

			describe<T>(b);
		}

		template <typename T, typename T2>
		inline void bind_identity(sqlite3 &connection, T2 &record)
		{
			primary_key_binder<T2> b = {	connection, record	};

			describe<T>(b);
		}
	}
}
