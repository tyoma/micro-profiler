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
#include "types.h"

#include <common/formatting.h>
#include <cstdint>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		struct column_definition_format_visitor
		{
			void operator ()(int T::*, const char *column_name)
			{	append_integer(column_name);	}

			void operator ()(unsigned int T::*, const char *column_name)
			{	append_integer(column_name);	}

			void operator ()(std::int64_t T::*, const char *column_name)
			{	append_integer(column_name);	}

			void operator ()(std::uint64_t T::*, const char *column_name)
			{	append_integer(column_name);	}

			void operator ()(std::string T::*, const char *column_name)
			{	append_text(column_name);	}

			void operator ()(double T::*, const char *column_name)
			{	append_real(column_name);	}

			template <typename U, typename F>
			void operator ()(const primary_key<U, F> &field, const char *column_name)
			{
				(*this)(field.field, column_name);
				column_definitions += " PRIMARY KEY ASC";
			}

			std::string &column_definitions;
			bool first;

		private:
			std::string &append_column(const char *column_name)
			{
				if (!first)
					column_definitions += ',';
				first = false;
				column_definitions.append(column_name);
				return column_definitions;
			}

			void append_integer(const char *column_name)
			{	append_column(column_name) += " INTEGER NOT NULL";	}

			void append_text(const char *column_name)
			{	append_column(column_name) += " TEXT NOT NULL";	}

			void append_real(const char *column_name)
			{	append_column(column_name) += " REAL NOT NULL";	}
		};


		template <typename T, typename F>
		struct format_column_visitor
		{
			format_column_visitor(F T::*field_, std::string &column_name_)
				: field(field_), column_name(column_name_)
			{	}

			template <typename U>
			void operator ()(F U::*field_, const char *column_name_) const
			{
				if (field_ == field)
					column_name.append(column_name_);
			}

			template <typename U> 
			void operator ()(U, const char *) const
			{	}

			F T::*field;
			std::string &column_name;
		};



		template <typename T, typename F>
		inline void format_expression(std::string &output, const column<T, F> &e, unsigned int &/*index*/)
		{
			format_column_visitor<T, F> v(e.field, output);
			describe<T>(v);
		}

		template <typename T>
		inline void format_expression(std::string &output, const parameter<T> &/*e*/, unsigned int &index)
		{	output += ':', itoa<10>(output, index++);	}

		template <typename L, typename R>
		inline void format_expression(std::string &output, const operator_<L, R> &e, unsigned int &index)
		{
			format_expression(output, e.lhs, index);
			output += e.literal;
			format_expression(output, e.rhs, index);
		}

		template <typename E>
		inline void format_expression(std::string &output, const E &e)
		{
			auto index = 1u;

			format_expression(output, e, index);
		}

		template <typename T>
		inline void format_create_table(std::string &output, const char *name)
		{
			column_definition_format_visitor<T> v = {	output, true	};

			output += "CREATE TABLE ";
			output += name;
			output += " (";
			describe<T>(v);
			output += ")";
		}
	}
}
