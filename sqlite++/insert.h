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

#include "binding.h"
#include "expression.h"
#include "misc.h"
#include "statement.h"
#include "types.h"

#include <cstdint>
#include <string>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		class inserter : statement
		{
		public:
			inserter(sqlite3 &connection, statement_ptr &&statement);

			template <typename T2>
			void operator ()(T2 &item);

		private:
			sqlite3 &_connection;
		};

		template <typename T>
		class insert_builder
		{
		public:
			insert_builder(const char *table_name);

			inserter<T> create_inserter(sqlite3 &connection);

			template <typename U>
			void operator ()(U);

			template <typename F>
			void operator ()(F field, const char *name);

			template <typename U, typename F>
			void operator ()(const primary_key<U, F> &field, const char *name);

		private:
			std::string _expression_text;
			int _index;
		};



		template <typename T>
		inline inserter<T>::inserter(sqlite3 &connection, statement_ptr &&statement_)
			: statement(std::move(statement_)), _connection(connection)
		{	}

		template <typename T>
		template <typename T2>
		inline void inserter<T>::operator ()(T2 &item)
		{
			bind_fields<T>(*this, item);
			execute();
			bind_identity<T>(_connection, item);
			reset();
		}


		template <typename T>
		inline insert_builder<T>::insert_builder(const char *table_name)
			: _expression_text("INSERT INTO "), _index(0)
		{
			_expression_text += table_name;
			_expression_text += " (";
			describe<T>(*this);
			_expression_text += ") VALUES (";
			while (_index--)
				_expression_text += _index ? "?," : "?";
			_expression_text += ")";
		}

		template <typename T>
		inline inserter<T> insert_builder<T>::create_inserter(sqlite3 &connection)
		{	return inserter<T>(connection, create_statement(connection, _expression_text.c_str()));	}

		template <typename T>
		template <typename U>
		inline void insert_builder<T>::operator ()(U)
		{	}

		template <typename T>
		template <typename F>
		inline void insert_builder<T>::operator ()(F /*field*/, const char *name)
		{
			if (_index++)
				_expression_text += ',';
			_expression_text += name;
		}

		template <typename T>
		template <typename U, typename F>
		inline void insert_builder<T>::operator ()(const primary_key<U, F> &, const char *)
		{	}
	}
}
