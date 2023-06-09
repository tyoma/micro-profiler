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
#include "format.h"
#include "misc.h"
#include "types.h"

#include <cstdint>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		struct record_reader
		{
			template <typename U>
			void operator ()(U)
			{	}

			template <typename FieldT, typename U>
			void operator ()(FieldT U::*field, const char *)
			{	record.*field = statement_.get(index++);	}

			template <typename U>
			void operator ()(std::string U::*field, const char *)
			{	record.*field = static_cast<const char *>(statement_.get(index++));	}

			template <typename U, typename F>
			void operator ()(const primary_key<U, F> &field, const char *name)
			{	(*this)(field.field, name);	}

			T &record;
			statement &statement_;
			int index;
		};

		template <typename T>
		class reader : statement
		{
		public:
			template <typename W>
			reader(statement_ptr &&statement, const W &where);
			reader(statement_ptr &&statement);

			bool operator ()(T& value);
		};

		template <typename T>
		class select_builder
		{
		public:
			select_builder(const char *table_name);

			template <typename U>
			void operator ()(U);

			template <typename F>
			void operator ()(F field, const char *name);

			reader<T> create_reader(sqlite3 &database) const;

			template <typename W>
			reader<T> create_reader(sqlite3 &database, const W &where) const;

		private:
			int _index;
			std::string _expression_text;
		};



		template <typename T>
		template <typename W>
		inline reader<T>::reader(statement_ptr &&statement_, const W &where)
			: statement(std::move(statement_))
		{	bind_parameters(*this, where);	}

		template <typename T>
		inline reader<T>::reader(statement_ptr &&statement_)
			: statement(std::move(statement_))
		{	}

		template <typename T>
		inline bool reader<T>::operator ()(T& record)
		{
			record_reader<T> rr = {	record, *this, 0	};

			return execute() ? describe<T>(rr), true : false;
		}


		template <typename T>
		inline select_builder<T>::select_builder(const char *table_name)
			: _index(0), _expression_text("SELECT")
		{
			describe<T>(*this);
			_expression_text += " FROM ";
			_expression_text += table_name;
		}

		template <typename T>
		template <typename U>
		inline void select_builder<T>::operator ()(U)
		{	}

		template <typename T>
		template <typename F>
		inline void select_builder<T>::operator ()(F /*field*/, const char *name)
		{
			_expression_text += _index++ ? ',' : ' ';
			_expression_text += name;
		}

		template <typename T>
		inline reader<T> select_builder<T>::create_reader(sqlite3 &database) const
		{	return reader<T>(create_statement(database, _expression_text.c_str()));	}

		template <typename T>
		template <typename W>
		inline reader<T> select_builder<T>::create_reader(sqlite3 &database, const W &where) const
		{
			auto expression_text = _expression_text;

			expression_text += " WHERE ";
			format_expression(expression_text, where);
			return reader<T>(create_statement(database, expression_text.c_str()), where);
		}
	}
}
