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

#include "sql_format.h"
#include "sql_misc.h"
#include "sql_types.h"

#include <cstdint>

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

		template <typename T>
		struct record_reader
		{
			void operator ()(int T::*field, const char *)
			{	record.*field = sqlite3_column_int(&statement, index++);	}

			void operator ()(unsigned int T::*field, const char *)
			{	record.*field = static_cast<unsigned int>(sqlite3_column_int(&statement, index++));	}

			void operator ()(std::int64_t T::*field, const char *)
			{	record.*field = sqlite3_column_int64(&statement, index++);	}

			void operator ()(std::uint64_t T::*field, const char *)
			{	record.*field = static_cast<std::uint64_t>(sqlite3_column_int64(&statement, index++));	}

			void operator ()(double T::*field, const char *)
			{	record.*field = sqlite3_column_double(&statement, index++);	}

			void operator ()(std::string T::*field, const char *)
			{	record.*field = (const char *)sqlite3_column_text(&statement, index++);	}

			template <typename F>
			void operator ()(const primary_key<T, F> &field, const char *name)
			{	(*this)(field.field, name);	}

			T &record;
			sqlite3_stmt &statement;
			int index;
		};

		template <typename T>
		class reader
		{
		public:
			reader(statement_ptr &&statement);

			bool operator ()(T& value);

		private:
			statement_ptr _statement;
		};

		template <typename T>
		class select_builder
		{
		public:
			select_builder(const char *table_name);

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
		inline reader<T>::reader(statement_ptr &&statement)
			: _statement(std::move(statement))
		{	}

		template <typename T>
		inline bool reader<T>::operator ()(T& record)
		{
			switch (sqlite3_step(_statement.get()))
			{
			case SQLITE_DONE:
				return false;

			case SQLITE_ROW:
				record_reader<T> rr = {	record, *_statement, 0	};

				describe<T>(rr);
				return true;
			}
			throw 0;
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

			auto s = create_statement(database, expression_text.c_str());

			bind_parameters(*s, where);
			return reader<T>(std::move(s));
		}
	}
}
