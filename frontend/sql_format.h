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
	}
}
