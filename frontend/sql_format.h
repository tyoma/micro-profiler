#pragma once

#include "sql_expression.h"

#include <common/formatting.h>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T, typename F>
		struct format_column_visitor
		{
			format_column_visitor(F T::*field_, std::string &column_name_)
				: field(field_), column_name(column_name_)
			{	}

			void operator ()(F T::*field_, const char *column_name_) const
			{
				if (field_ == field)
					column_name.append(column_name_);
			}

			template <typename U> 
			void operator ()(U, const char *) const
			{	}

			F T::*field;
			std::string &column_name;

		private:
			format_column_visitor(const format_column_visitor &other);
			void operator =(const format_column_visitor &rhs);
		};

		struct format_visitor
		{
			format_visitor(std::string &output_)
				: output(output_), next_index(1)
			{	}

			template <typename T>
			void operator ()(const parameter<T> &/*e*/)
			{	output += ':', itoa<10>(output, next_index++);	}

			template <typename T, typename F>
			void operator ()(const column<T, F> &e)
			{
				format_column_visitor<T, F> v(e.field, output);
				describe(v, static_cast<T *>(nullptr));
			}

			template <typename L, typename R>
			void operator ()(const operator_<L, R> &e)
			{
				(*this)(e.lhs);
				output += e.literal;
				(*this)(e.rhs);
			}

			std::string &output;
			unsigned int next_index;

		private:
			format_visitor(const format_visitor &other);
			void operator =(const format_visitor &rhs);
		};
	}
}
