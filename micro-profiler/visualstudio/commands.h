//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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

namespace micro_profiler
{
	namespace integration
	{
		class dispatch;

		struct project_command
		{
			enum state_flags { supported = 1, enabled = 2, visible = 4, checked = 8 };

			virtual ~project_command() throw() { }

			virtual int query_state(const dispatch &dte_project) const = 0;
			virtual void exec(const dispatch &dte_project) const = 0;
		};

		struct toggle_profiling : project_command
		{
			virtual int query_state(const dispatch &dte_project) const;
			virtual void exec(const dispatch &dte_project) const;
		};

		struct remove_profiling_support : project_command
		{
			virtual int query_state(const dispatch &dte_project) const;
			virtual void exec(const dispatch &dte_project) const;
		};
	}
}
