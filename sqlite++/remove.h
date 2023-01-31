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

#include "binding.h"
#include "format.h"
#include "statement.h"

#include <functional>

namespace micro_profiler
{
	namespace sql
	{
		class remover : statement
		{
		public:
			template <typename W>
			remover(statement_ptr &&statement, const W &where);

			using statement::execute;
			void reset();

		private:
			std::function<void ()> _reset_bindings;
		};

		class remove_builder
		{
		public:
			remove_builder(const char *table_name);

			template <typename W>
			remover create_statement(sqlite3 &database, const W &where) const;

		private:
			std::string _expression_text;
		};



		template <typename W>
		inline remover::remover(statement_ptr &&statement_, const W &where)
			: statement(std::move(statement_)), _reset_bindings([this, where] {	bind_parameters(*this, where);	})
		{	_reset_bindings();	}

		inline void remover::reset()
		{
			statement::reset();
			_reset_bindings();
		}


		inline remove_builder::remove_builder(const char *table_name)
			: _expression_text("DELETE FROM ")
		{	_expression_text += table_name;	}

		template <typename W>
		inline remover remove_builder::create_statement(sqlite3 &database, const W &where) const
		{
			auto expression_text = _expression_text;

			expression_text += " WHERE ";
			format_expression(expression_text, where);
			return remover(sql::create_statement(database, expression_text.c_str()), where);
		}
	}
}
