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
#include <tuple>

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
			select_builder();

			reader<T> create_reader(sqlite3 &database) const;

			template <typename W>
			reader<T> create_reader(sqlite3 &database, const W &where) const;

		private:
			int _index;
			std::string _expression_text;
		};



		template <typename T>
		inline void read_field(T &record, statement &statement_)
		{
			record_reader<T> rr = {	record, statement_, 0	};

			describe<T>(rr);
		}

		template <typename T1, typename T2>
		inline void read_field(std::tuple<T1, T2> &record, statement &statement_)
		{
			record_reader<T1> rr0 = {	std::get<0>(record), statement_, 0	};
			describe<T1>(rr0);
			record_reader<T2> rr1 = {	std::get<1>(record), statement_, rr0.index	};
			describe<T2>(rr1);
		}

		template <typename T1, typename T2, typename T3>
		inline void read_field(std::tuple<T1, T2, T3> &record, statement &statement_)
		{
			record_reader<T1> rr0 = {	std::get<0>(record), statement_, 0	};
			describe<T1>(rr0);
			record_reader<T2> rr1 = {	std::get<1>(record), statement_, rr0.index	};
			describe<T2>(rr1);
			record_reader<T3> rr2 = {	std::get<2>(record), statement_, rr1.index	};
			describe<T3>(rr2);
		}


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
		{	return execute() ? read_field(record, *this), true : false;	}


		template <typename T>
		inline select_builder<T>::select_builder()
			: _expression_text("SELECT ")
		{
			format_select_list(_expression_text, static_cast<T *>(nullptr));
			_expression_text += " FROM ";
			format_table_source(_expression_text, static_cast<T *>(nullptr));
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
