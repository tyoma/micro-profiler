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

#include "misc.h"

#include <cstdint>
#include <functional>
#include <stdexcept>

namespace micro_profiler
{
	namespace sql
	{
		struct execution_error : std::runtime_error
		{
			execution_error(int code_);

			int code;
		};

		class statement
		{
		public:
			class field_accessor;

		public:
			statement(statement_ptr &&underlying);

			void reset();
			bool execute();

			void bind(int index, std::int32_t value);
			void bind(int index, std::uint32_t value);
			void bind(int index, std::int64_t value);
			void bind(int index, std::uint64_t value);
			void bind(int index, double value);
			void bind(int index, const char *value);
			void bind(int index, const std::string &value);
			field_accessor get(int index) const;

		private:
			statement_ptr _underlying;
		};

		class statement::field_accessor
		{
		public:
			field_accessor(sqlite3_stmt &s, int index);

			operator std::int32_t() const;
			operator std::uint32_t() const;
			operator std::int64_t() const;
			operator std::uint64_t() const;
			operator double() const;
			operator const char *() const;

		private:
			sqlite3_stmt &_statement;
			int _index;
		};



		inline statement::statement(statement_ptr &&underlying)
			: _underlying(std::move(underlying))
		{	}

		inline void statement::reset()
		{
			sqlite3_reset(_underlying.get());
			sqlite3_clear_bindings(_underlying.get());
		}

		inline bool statement::execute()
		{
			switch (auto result = sqlite3_step(_underlying.get()))
			{
			case SQLITE_DONE: return false;
			case SQLITE_ROW: return true;
			default: throw execution_error(result);
			}
		}

		inline void statement::bind(int index, std::int32_t value)
		{	sqlite3_bind_int(_underlying.get(), index, value);	}

		inline void statement::bind(int index, std::uint32_t value)
		{	bind(index, static_cast<std::int32_t>(value));	}

		inline void statement::bind(int index, std::int64_t value)
		{	sqlite3_bind_int64(_underlying.get(), index, value);	}

		inline void statement::bind(int index, std::uint64_t value)
		{	bind(index, static_cast<std::int64_t>(value));	}

		inline void statement::bind(int index, double value)
		{	sqlite3_bind_double(_underlying.get(), index, value);	}

		inline void statement::bind(int index, const char *value)
		{	sqlite3_bind_text(_underlying.get(), index, value, -1, SQLITE_TRANSIENT);	}

		inline void statement::bind(int index, const std::string &value)
		{	bind(index, value.c_str()); }

		inline statement::field_accessor statement::get(int index) const
		{	return statement::field_accessor(*_underlying, index);	}


		inline statement::field_accessor::field_accessor(sqlite3_stmt &s, int index)
			: _statement(s), _index(index)
		{	}

		inline statement::field_accessor::operator std::int32_t() const
		{	return sqlite3_column_int(&_statement, _index);	}

		inline statement::field_accessor::operator std::uint32_t() const
		{	return static_cast<std::int32_t>(*this);	}

		inline statement::field_accessor::operator std::int64_t() const
		{	return sqlite3_column_int64(&_statement, _index);	}

		inline statement::field_accessor::operator std::uint64_t() const
		{	return static_cast<std::int64_t>(*this);	}

		inline statement::field_accessor::operator double() const
		{	return sqlite3_column_double(&_statement, _index);	}

		inline statement::field_accessor::operator const char *() const
		{	return reinterpret_cast<const char *>(sqlite3_column_text(&_statement, _index));	}


		inline execution_error::execution_error(int code_)
			: std::runtime_error(""), code(code_)
		{	}
	}
}
