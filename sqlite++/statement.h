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
#include "misc.h"

#include <functional>

namespace micro_profiler
{
	namespace sql
	{
		class statement
		{
		public:
			template <typename W>
			statement(statement_ptr &&underlying, const W &where);

			void reset();
			void execute();

		private:
			statement_ptr _underlying;
			std::function<void ()> _reset_bindings;
		};



		template <typename W>
		inline statement::statement(statement_ptr &&underlying, const W &where)
			: _underlying(std::move(underlying))
		{
			auto reset_bindings_ = [this, where] {	bind_parameters(*_underlying, where);	};

			_reset_bindings = reset_bindings_;
			reset_bindings_();
		}

		inline void statement::reset()
		{
			sqlite3_reset(_underlying.get());
			_reset_bindings();
		}

		inline void statement::execute()
		{	sqlite3_step(_underlying.get());	}
	}
}
